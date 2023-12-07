#include "Scheduler.hpp"

#include "../http/http.hpp"
#include "../log/Exception.hpp"
#include "../log/logger.hpp"
#include "../socket/Client.hpp"
#include "../userRing/Completion.hpp"
#include "../userRing/Submission.hpp"

#include <cstring>

Scheduler::Scheduler() noexcept : userRing{Scheduler::initializeUserRing()}, server{0}, timer{1} {
    this->userRing->registerSelfFileDescriptor();
    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());
    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const std::array<int, 2> fileDescriptors{Server::create(8080), Timer::create()};
    this->userRing->updateFileDescriptors(0, fileDescriptors);

    this->database.connect({}, "AomaYple", "38820233", "webServer", 0, {}, 0);
}

Scheduler::~Scheduler() {
    this->closeAll();

    const std::lock_guard lockGuard{Scheduler::lock};

    auto result{std::ranges::find(Scheduler::userRingFileDescriptors, this->userRing->getSelfFileDescriptor())};
    *result = -1;

    if (Scheduler::sharedUserRingFileDescriptor == this->userRing->getSelfFileDescriptor()) {
        Scheduler::sharedUserRingFileDescriptor = -1;

        result = std::ranges::find_if(Scheduler::userRingFileDescriptors,
                                      [](const int fileDescriptor) { return fileDescriptor != -1; });

        if (result != Scheduler::userRingFileDescriptors.cend()) Scheduler::sharedUserRingFileDescriptor = *result;
    }

    Scheduler::instance = false;
}

auto Scheduler::registerSignal(std::source_location sourceLocation) noexcept -> void {
    struct sigaction signalAction {};
    signalAction.sa_handler = [](int) noexcept { Scheduler::switcher.clear(std::memory_order_relaxed); };

    if (sigaction(SIGTERM, &signalAction, nullptr) == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }

    if (sigaction(SIGINT, &signalAction, nullptr) == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Scheduler::run() noexcept -> void {
    this->server.setAcceptGenerator(this->accept());
    this->server.resumeAccept();

    this->timer.setTimingGenerator(this->timing());
    this->timer.resumeTiming();

    while (Scheduler::switcher.test(std::memory_order_relaxed)) {
        this->userRing->submitWait(1);
        this->userRing->traverseCompletion([this](const Completion &completion) noexcept { this->frame(completion); });
    }

    this->cancelAll();
}

auto Scheduler::initializeUserRing(std::source_location sourceLocation) noexcept -> std::shared_ptr<UserRing> {
    if (Scheduler::instance) {
        logger::push(Log{Log::Level::fatal, "one scheduler instance per thread", sourceLocation});

        std::terminate();
    }
    Scheduler::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    const std::lock_guard lockGuard{Scheduler::lock};

    if (Scheduler::sharedUserRingFileDescriptor != -1) {
        params.wq_fd = Scheduler::sharedUserRingFileDescriptor;
        params.flags |= IORING_SETUP_ATTACH_WQ;
    }

    std::shared_ptr<UserRing> userRing{std::make_shared<UserRing>(1024, params)};

    if (Scheduler::sharedUserRingFileDescriptor == -1)
        Scheduler::sharedUserRingFileDescriptor = userRing->getSelfFileDescriptor();

    const auto result{std::ranges::find(Scheduler::userRingFileDescriptors, -1)};
    *result = userRing->getSelfFileDescriptor();

    userRing->registerCpu(result - Scheduler::userRingFileDescriptors.cbegin());

    return userRing;
}

auto Scheduler::cancelAll() noexcept -> void {
    this->server.setCancelGenerator(this->cancelServer());
    this->server.resumeCancel();

    this->timer.setCancelGenerator(this->cancelTimer());
    this->timer.resumeCancel();

    for (std::pair<const int, Client> &element: this->clients) {
        Client &client{element.second};

        client.setCancelGenerator(this->cancelClient(client));
        client.resumeCancel();
    }

    this->userRing->submitWait(4 + this->clients.size() * 2);
    this->userRing->traverseCompletion([this](const Completion &completion) noexcept { this->frame(completion); });
}

auto Scheduler::closeAll() noexcept -> void {
    this->server.setCloseGenerator(this->closeServer());
    this->server.resumeClose();

    this->timer.setCloseGenerator(this->closeTimer());
    this->timer.resumeClose();

    for (std::pair<const int, Client> &element: this->clients) {
        Client &client{element.second};

        client.setCloseGenerator(this->closeClient(client));
        client.resumeClose();
    }

    this->userRing->submitWait(2 + this->clients.size());
    this->userRing->traverseCompletion([this](const Completion &completion) noexcept { this->frame(completion); });
}

auto Scheduler::frame(const Completion &completion, std::source_location sourceLocation) noexcept -> void {
    const Event event{std::bit_cast<Event>(completion.event)};
    const Outcome outcome{completion.outcome};

    try {
        switch (event.type) {
            case Event::Type::accept:
                if (outcome.result >= 0 || std::abs(outcome.result) != ECANCELED) {
                    this->server.setAwaiterOutcome(outcome);
                    this->server.resumeAccept();
                }

                break;
            case Event::Type::timing:
                if (outcome.result >= 0 || std::abs(outcome.result) != ECANCELED) {
                    this->timer.setAwaiterOutcome(outcome);
                    this->timer.resumeTiming();
                }

                break;
            case Event::Type::receive:
                if (outcome.result >= 0 || std::abs(outcome.result) != ECANCELED) {
                    Client &client{this->clients.at(event.fileDescriptor)};

                    client.setAwaiterOutcome(outcome);
                    client.resumeReceive();
                }

                break;
            case Event::Type::send:
                if (!(outcome.flags & IORING_CQE_F_NOTIF)) {
                    Client &client{this->clients.at(event.fileDescriptor)};

                    client.setAwaiterOutcome(outcome);
                    client.resumeSend();
                }

                break;
            case Event::Type::cancel:
                if (event.fileDescriptor == this->server.getFileDescriptorIndex()) {
                    this->server.setAwaiterOutcome(outcome);
                    this->server.resumeCancel();
                } else if (event.fileDescriptor == this->timer.getFileDescriptorIndex()) {
                    this->timer.setAwaiterOutcome(outcome);
                    this->timer.resumeCancel();
                } else {
                    Client &client{this->clients.at(event.fileDescriptor)};

                    client.setAwaiterOutcome(outcome);
                    client.resumeCancel();
                }

                break;
            case Event::Type::close:
                if (event.fileDescriptor == this->server.getFileDescriptorIndex()) {
                    this->server.setAwaiterOutcome(outcome);
                    this->server.resumeClose();
                } else if (event.fileDescriptor == this->timer.getFileDescriptorIndex()) {
                    this->timer.setAwaiterOutcome(outcome);
                    this->timer.resumeClose();
                } else {
                    const auto result{this->clients.find(event.fileDescriptor)};
                    if (result != this->clients.cend()) {
                        result->second.setAwaiterOutcome(outcome);

                        try {
                            result->second.resumeClose();
                        } catch (Exception &exception) { logger::push(std::move(exception.getLog())); }

                        this->clients.erase(result);
                    } else
                        throw Exception{Log{Log::Level::error, "cannot find client", sourceLocation}};
                }

                break;
        }
    } catch (Exception &exception) { logger::push(std::move(exception.getLog())); }
}

auto Scheduler::accept(std::source_location sourceLocation) -> Generator {
    this->userRing->addSubmission(this->server.getAcceptSubmission());

    while (true) {
        const Outcome outcome{co_await this->server.accept()};
        if (outcome.result >= 0) {
            this->clients.emplace(outcome.result,
                                  Client{outcome.result, BufferRing{1, 1024, outcome.result, this->userRing}, 600});

            Client &client{this->clients.at(outcome.result)};

            this->timer.add(outcome.result, client.getSeconds());

            client.setReceiveGenerator(this->receive(client));
            client.resumeReceive();
        } else
            throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};

        if (!(outcome.flags & IORING_CQE_F_MORE))
            throw Exception{Log{Log::Level::fatal, "cannot accept more connections", sourceLocation}};
    }
}

auto Scheduler::timing(std::source_location sourceLocation) -> Generator {
    while (true) {
        this->userRing->addSubmission(this->timer.getTimingSubmission());

        const Outcome outcome{co_await this->timer.timing()};
        if (outcome.result == sizeof(unsigned long)) {
            for (const int fileDescriptor: this->timer.clearTimeout()) {
                Client &client{this->clients.at(fileDescriptor)};

                client.setCancelGenerator(this->cancelClient(client));
                client.resumeCancel();
            }
        } else
            throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};
    }
}

auto Scheduler::receive(Client &client, std::source_location sourceLocation) -> Generator {
    this->userRing->addSubmission(client.getReceiveSubmission());

    while (true) {
        const Outcome outcome{co_await client.receive()};

        if (outcome.result > 0) {
            client.writeToBuffer(client.getBufferRingData(outcome.flags >> IORING_CQE_BUFFER_SHIFT, outcome.result));

            if (!(outcome.flags & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(client.getFileDescriptorIndex(), client.getSeconds());

                client.setSendGenerator(this->send(client));
                client.resumeSend();
            }
        } else {
            this->timer.remove(client.getFileDescriptorIndex());

            client.setCancelGenerator(this->cancelClient(client));
            client.resumeCancel();

            throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
        }

        if (!(outcome.flags & IORING_CQE_F_MORE))
            throw Exception{Log{Log::Level::error, "cannot receive more data", sourceLocation}};
    }
}

auto Scheduler::send(Client &client, std::source_location sourceLocation) -> Generator {
    const std::span<const std::byte> request{client.readFromBuffer()};
    std::vector<std::byte> response{http::parse(
            std::string_view{reinterpret_cast<const char *>(request.data()), request.size()}, this->database)};

    client.clearBuffer();
    client.writeToBuffer(response);

    this->userRing->addSubmission(client.getSendSubmission());
    const Outcome outcome{co_await client.send()};

    client.clearBuffer();

    if (outcome.result <= 0) {
        this->timer.remove(client.getFileDescriptorIndex());

        client.setCancelGenerator(this->cancelClient(client));
        client.resumeCancel();

        throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
    }
}

auto Scheduler::cancelClient(Client &client, std::source_location sourceLocation) -> Generator {
    this->userRing->addSubmission(client.getCancelSubmission());
    const Outcome outcome{co_await client.cancel()};

    client.setCloseGenerator(this->closeClient(client));
    client.resumeClose();

    if (outcome.result < 0)
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::closeClient(const Client &client, std::source_location sourceLocation) -> Generator {
    this->userRing->addSubmission(client.getCloseSubmission());

    const Outcome outcome{co_await client.close()};
    if (outcome.result < 0)
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::cancelServer(std::source_location sourceLocation) -> Generator {
    this->userRing->addSubmission(this->server.getCancelSubmission());

    const Outcome outcome{co_await this->server.cancel()};
    if (outcome.result < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::closeServer(std::source_location sourceLocation) const -> Generator {
    this->userRing->addSubmission(this->server.getCloseSubmission());

    const Outcome outcome{co_await this->server.close()};
    if (outcome.result < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::cancelTimer(std::source_location sourceLocation) const -> Generator {
    this->userRing->addSubmission(this->timer.getCancelSubmission());

    const Outcome outcome{co_await this->timer.cancel()};
    if (outcome.result < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::closeTimer(std::source_location sourceLocation) const -> Generator {
    this->userRing->addSubmission(this->timer.getCloseSubmission());

    const Outcome outcome{co_await this->timer.close()};
    if (outcome.result < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

constinit thread_local bool Scheduler::instance{false};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedUserRingFileDescriptor{-1};
std::vector<int> Scheduler::userRingFileDescriptors{std::vector<int>(std::jthread::hardware_concurrency(), -1)};
constinit std::atomic_flag Scheduler::switcher{true};
