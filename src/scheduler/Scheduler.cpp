#include "Scheduler.hpp"

#include "../http/Http.hpp"
#include "../log/Exception.hpp"
#include "../log/Logger.hpp"
#include "../socket/Client.hpp"
#include "../userRing/Completion.hpp"
#include "../userRing/Event.hpp"

#include <csignal>
#include <cstring>

Scheduler::Scheduler()
    : userRing{Scheduler::initializeUserRing()}, bufferRing{1024, 1024, 0, this->userRing}, server{0}, timer{1},
      database{{}, "AomaYple", "38820233", "webServer", 0, {}, 0} {
    this->userRing->registerSelfFileDescriptor();
    this->userRing->registerSparseFileDescriptors(UserRing::getFileDescriptorLimit());
    this->userRing->allocateFileDescriptorRange(2, UserRing::getFileDescriptorLimit() - 2);

    const std::array<unsigned int, 2> fileDescriptors{Server::create(8080), Timer::create()};

    this->userRing->updateFileDescriptors(0, fileDescriptors);
}

Scheduler::~Scheduler() {
    this->closeAll();

    const std::lock_guard lockGuard{Scheduler::lock};

    auto result{std::ranges::find(Scheduler::userRingFileDescriptors,
                                  static_cast<int>(this->userRing->getSelfFileDescriptor()))};
    *result = -1;

    if (Scheduler::sharedUserRingFileDescriptor == static_cast<int>(this->userRing->getSelfFileDescriptor())) {
        Scheduler::sharedUserRingFileDescriptor = -1;

        result = std::ranges::find_if(Scheduler::userRingFileDescriptors,
                                      [](const int fileDescriptor) { return fileDescriptor != -1; });

        if (result != Scheduler::userRingFileDescriptors.cend()) Scheduler::sharedUserRingFileDescriptor = *result;
    }

    Scheduler::instance = false;
}

auto Scheduler::registerSignal(std::source_location sourceLocation) -> void {
    struct sigaction signalAction {};
    signalAction.sa_handler = [](int) noexcept {
        Scheduler::switcher.clear(std::memory_order_relaxed);

        Logger::stop();
    };

    if (sigaction(SIGTERM, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    if (sigaction(SIGINT, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

auto Scheduler::run() -> void {
    Generator generator{this->accept()};
    generator.resume();
    this->server.setAcceptGenerator(std::move(generator));

    generator = this->timing();
    generator.resume();
    this->timer.setTimingGenerator(std::move(generator));

    while (Scheduler::switcher.test(std::memory_order_relaxed)) {
        this->userRing->submitWait(1);

        const unsigned int completionCount{
                this->userRing->forEachCompletion([this](const io_uring_cqe *cqe) { this->frame(cqe); })};

        this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
    }

    this->cancelAll();
}

auto Scheduler::initializeUserRing(std::source_location sourceLocation) -> std::shared_ptr<UserRing> {
    if (Scheduler::instance)
        throw Exception{Log{Log::Level::fatal, "one scheduler instance per thread", sourceLocation}};
    Scheduler::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    std::shared_ptr<UserRing> userRing;

    {
        const std::lock_guard lockGuard{Scheduler::lock};

        if (Scheduler::sharedUserRingFileDescriptor != -1) {
            params.wq_fd = Scheduler::sharedUserRingFileDescriptor;
            params.flags |= IORING_SETUP_ATTACH_WQ;
        }

        userRing = std::make_shared<UserRing>(1024, params);

        if (Scheduler::sharedUserRingFileDescriptor == -1)
            Scheduler::sharedUserRingFileDescriptor = static_cast<int>(userRing->getSelfFileDescriptor());

        const auto result{std::ranges::find(Scheduler::userRingFileDescriptors, -1)};
        *result = static_cast<int>(userRing->getSelfFileDescriptor());

        userRing->registerCpu(std::distance(Scheduler::userRingFileDescriptors.begin(), result));
    }

    return userRing;
}

auto Scheduler::cancelAll() -> void {
    Generator generator{this->cancelServer()};
    generator.resume();
    this->server.setCancelGenerator(std::move(generator));

    generator = this->cancelTimer();
    generator.resume();
    this->timer.setCancelGenerator(std::move(generator));

    for (std::pair<const unsigned int, Client> &client: this->clients) {
        generator = this->cancelClient(client.second);
        generator.resume();
        client.second.setCancelGenerator(std::move(generator));
    }

    this->userRing->submitWait(this->clients.size() * 2 + 3);

    const unsigned int completionCount{
            this->userRing->forEachCompletion([this](const io_uring_cqe *cqe) { this->frame(cqe); })};

    this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
}

auto Scheduler::closeAll() -> void {
    Generator generator{this->closeServer()};
    generator.resume();
    this->server.setCloseGenerator(std::move(generator));

    generator = this->closeTimer();
    generator.resume();
    this->timer.setCloseGenerator(std::move(generator));

    for (std::pair<const unsigned int, Client> &client: this->clients) {
        generator = this->closeClient(client.second);
        generator.resume();
        client.second.setCloseGenerator(std::move(generator));
    }

    this->userRing->submitWait(2 + this->clients.size());

    const unsigned int completionCount{
            this->userRing->forEachCompletion([this](const io_uring_cqe *cqe) { this->frame(cqe); })};

    this->bufferRing.advanceCompletionBufferRingBuffer(completionCount);
}

auto Scheduler::frame(const io_uring_cqe *cqe) -> void {
    const Completion completion{cqe};

    const unsigned long completionUserData{completion.getUserData()};
    const Event event{std::bit_cast<Event>(completionUserData)};

    const std::pair<int, unsigned int> result{completion.getResult(), completion.getFlags()};

    switch (event.type) {
        case Event::Type::accept:
            if (result.first >= 0 || std::abs(result.first) != ECANCELED) this->server.resumeAccept(result);

            break;
        case Event::Type::timeout:
            if (result.first >= 0 || std::abs(result.first) != ECANCELED) this->timer.resumeTiming(result);

            break;
        case Event::Type::receive:
            if (result.first >= 0 || std::abs(result.first) != ECANCELED) {
                Client &client{this->clients.at(event.fileDescriptor)};

                try {
                    client.resumeReceive(result);
                } catch (Exception &exception) {
                    client.setReceiveGenerator(Generator{});

                    Logger::produce(exception.getLog());
                }
            }

            break;
        case Event::Type::send:
            if (!(result.second & IORING_CQE_F_NOTIF)) {
                Client &client{this->clients.at(event.fileDescriptor)};

                try {
                    client.resumeSend(result);
                } catch (Exception &exception) { Logger::produce(exception.getLog()); }

                client.setSendGenerator(Generator{});
            }

            break;
        case Event::Type::cancel:
            if (event.fileDescriptor == this->server.getFileDescriptorIndex()) {
                this->server.resumeCancel(result);

                this->server.setCancelGenerator(Generator{});
            } else if (event.fileDescriptor == this->timer.getFileDescriptorIndex()) {
                this->timer.resumeCancel(result);

                this->timer.setCancelGenerator(Generator{});
            } else {
                Client &client{this->clients.at(event.fileDescriptor)};

                try {
                    client.resumeCancel(result);
                } catch (Exception &exception) { Logger::produce(exception.getLog()); }

                client.setCancelGenerator(Generator{});
            }

            break;
        case Event::Type::close:
            if (event.fileDescriptor == this->server.getFileDescriptorIndex()) {
                this->server.resumeClose(result);

                this->server.setCloseGenerator(Generator{});
            } else if (event.fileDescriptor == this->timer.getFileDescriptorIndex()) {
                this->timer.resumeClose(result);

                this->timer.setCloseGenerator(Generator{});
            } else {
                auto findResult{this->clients.find(event.fileDescriptor)};

                if (findResult != this->clients.cend()) {
                    try {
                        findResult->second.resumeClose(result);
                    } catch (Exception &exception) { Logger::produce(exception.getLog()); }

                    this->clients.erase(findResult);
                }
            }

            break;
    }
}

auto Scheduler::accept(std::source_location sourceLocation) -> Generator {
    this->server.startAccept(this->userRing->getSqe());

    while (true) {
        const std::pair<int, unsigned int> result{co_await this->server.accept()};

        if (result.first >= 0) {
            this->clients.emplace(result.first, Client{static_cast<unsigned int>(result.first), 3600});

            Client &client{this->clients.at(result.first)};

            this->timer.add(result.first, client.getTimeout());

            Generator generator{this->receive(client)};
            generator.resume();
            client.setReceiveGenerator(std::move(generator));
        } else
            throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};

        if (!(result.second & IORING_CQE_F_MORE))
            throw Exception{Log{Log::Level::fatal, "cannot accept more connections", sourceLocation}};
    }
}

auto Scheduler::timing(std::source_location sourceLocation) -> Generator {
    while (true) {
        const std::pair<int, unsigned int> result{co_await this->timer.timing(this->userRing->getSqe())};

        if (result.first == sizeof(unsigned long)) {
            for (const unsigned int fileDescriptor: this->timer.clearTimeout()) {
                Client &client{this->clients.at(fileDescriptor)};

                Generator generator{this->cancelClient(client)};
                generator.resume();
                client.setCancelGenerator(std::move(generator));
            }
        } else
            throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};
    }
}

auto Scheduler::receive(Client &client, std::source_location sourceLocation) -> Generator {
    client.startReceive(this->userRing->getSqe(), this->bufferRing.getId());

    while (true) {
        const std::pair<int, unsigned int> result{co_await client.receive()};

        if (result.first > 0) {
            client.writeData(this->bufferRing.getData(result.second >> IORING_CQE_BUFFER_SHIFT, result.first));

            if (!(result.second & IORING_CQE_F_SOCK_NONEMPTY)) {
                this->timer.update(client.getFileDescriptorIndex(), client.getTimeout());

                Generator generator{this->send(client)};
                generator.resume();
                client.setSendGenerator(std::move(generator));
            }
        } else {
            this->timer.remove(client.getFileDescriptorIndex());

            Generator generator{this->cancelClient(client)};
            generator.resume();
            client.setCancelGenerator(std::move(generator));

            throw Exception{Log{Log::Level::error, std::strerror(std::abs(result.first)), sourceLocation}};
        }

        if (!(result.second & IORING_CQE_F_MORE))
            throw Exception{Log{Log::Level::error, "cannot receive more data", sourceLocation}};
    }
}

auto Scheduler::send(Client &client, std::source_location sourceLocation) -> Generator {
    const std::span<const std::byte> request{client.readData()};
    std::vector<std::byte> response{Http::parse(
            std::string_view{reinterpret_cast<const char *>(request.data()), request.size()}, this->database)};

    client.clearBuffer();

    const std::pair<int, unsigned int> result{co_await client.send(this->userRing->getSqe(), std::move(response))};

    client.clearBuffer();

    if (result.first <= 0) {
        this->timer.remove(client.getFileDescriptorIndex());

        Generator generator{this->cancelClient(client)};
        generator.resume();
        client.setCancelGenerator(std::move(generator));

        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result.first)), sourceLocation}};
    }
}

auto Scheduler::cancelClient(Client &client, std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await client.cancel(this->userRing->getSqe())};

    Generator generator{this->closeClient(client)};
    generator.resume();
    client.setCloseGenerator(std::move(generator));

    if (result.first < 0)
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result.first)), sourceLocation}};
}

auto Scheduler::closeClient(const Client &client, std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await client.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result.first)), sourceLocation}};
}

auto Scheduler::cancelServer(std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await this->server.cancel(this->userRing->getSqe())};

    if (result.first < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};
}

auto Scheduler::closeServer(std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await this->server.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};
}

auto Scheduler::cancelTimer(std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await this->timer.cancel(this->userRing->getSqe())};

    if (result.first < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};
}

auto Scheduler::closeTimer(std::source_location sourceLocation) const -> Generator {
    const std::pair<int, unsigned int> result{co_await this->timer.close(this->userRing->getSqe())};

    if (result.first < 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result.first)), sourceLocation}};
}

constinit thread_local bool Scheduler::instance{false};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedUserRingFileDescriptor{-1};
std::vector<int> Scheduler::userRingFileDescriptors{std::vector<int>(std::jthread::hardware_concurrency(), -1)};
constinit std::atomic_flag Scheduler::switcher{true};
