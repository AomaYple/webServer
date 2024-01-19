#include "Scheduler.hpp"

#include "../fileDescriptor/Client.hpp"
#include "../log/Exception.hpp"
#include "../log/logger.hpp"
#include "../ring/Completion.hpp"

#include <cstring>

auto Scheduler::registerSignal(std::source_location sourceLocation) -> void {
    struct sigaction signalAction {};
    signalAction.sa_handler = [](int) noexcept { Scheduler::switcher.clear(std::memory_order_relaxed); };

    if (sigaction(SIGTERM, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    if (sigaction(SIGINT, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

Scheduler::Scheduler() {
    this->ring->registerSelfFileDescriptor();

    {
        const std::lock_guard lockGuard{Scheduler::lock};

        const auto result{std::ranges::find(Scheduler::ringFileDescriptors, this->ring->getFileDescriptor())};
        this->ring->registerCpu(std::distance(Scheduler::ringFileDescriptors.begin(), result));
    }

    this->ring->registerSparseFileDescriptor(Ring::getFileDescriptorLimit());
    this->ring->allocateFileDescriptorRange(2, Ring::getFileDescriptorLimit() - 2);

    const std::array<int, 2> fileDescriptors{Server::create(8080), Timer::create()};
    this->ring->updateFileDescriptors(0, fileDescriptors);
}

Scheduler::~Scheduler() {
    const std::lock_guard lockGuard{Scheduler::lock};

    auto result{std::ranges::find(Scheduler::ringFileDescriptors, this->ring->getFileDescriptor())};
    *result = -1;

    if (Scheduler::sharedRingFileDescriptor == this->ring->getFileDescriptor()) {
        Scheduler::sharedRingFileDescriptor = -1;

        result = std::ranges::find_if(Scheduler::ringFileDescriptors,
                                      [](const int fileDescriptor) noexcept { return fileDescriptor != -1; });
        if (result != Scheduler::ringFileDescriptors.cend()) Scheduler::sharedRingFileDescriptor = *result;
    }

    Scheduler::instance = false;
}

auto Scheduler::run() -> void {
    this->server.setGenerator(this->accept());
    this->timer.setGenerator(this->timing());

    while (Scheduler::switcher.test(std::memory_order::relaxed)) {
        this->ring->poll([this](const Completion &completion) {
            switch (completion.event.type) {
                case Event::Type::accept:
                    this->server.resumeGenerator(completion.outcome);
                    break;
                case Event::Type::read:
                    this->timer.resumeGenerator(completion.outcome);
                    this->timer.setGenerator(this->timing());

                    break;
                case Event::Type::receive:
                    try {
                        const auto result{this->clients.find(completion.event.fileDescriptor)};
                        if (result != this->clients.cend()) result->second.resumeFirstGenerator(completion.outcome);
                    } catch (Exception &exception) {
                        logger::push(std::move(exception.getLog()));

                        this->timer.remove(completion.event.fileDescriptor);
                        this->clients.erase(completion.event.fileDescriptor);
                    }

                    break;
                case Event::Type::send:
                    if (!(completion.outcome.flags & IORING_CQE_F_NOTIF)) {
                        try {
                            const auto result{this->clients.find(completion.event.fileDescriptor)};
                            if (result != this->clients.cend()) {
                                result->second.resumeSecondGenerator(completion.outcome);
                                result->second.setSecondGenerator(Generator{nullptr});
                            }
                        } catch (Exception &exception) {
                            logger::push(std::move(exception.getLog()));

                            this->timer.remove(completion.event.fileDescriptor);
                            this->clients.erase(completion.event.fileDescriptor);
                        }
                    }
                    break;
                case Event::Type::close:
                    Scheduler::closed(completion.event.fileDescriptor, completion.outcome.result);
                    break;
            }
        });
    }
}

auto Scheduler::initializeRing(std::source_location sourceLocation) -> std::shared_ptr<Ring> {
    if (Scheduler::instance)
        throw Exception{Log{Log::Level::fatal, "one thread can only have one Scheduler", sourceLocation}};
    Scheduler::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

    const std::lock_guard lockGuard{Scheduler::lock};

    if (Scheduler::sharedRingFileDescriptor != -1) {
        params.wq_fd = Scheduler::sharedRingFileDescriptor;
        params.flags |= IORING_SETUP_ATTACH_WQ;
    }

    std::shared_ptr<Ring> ring{std::make_shared<Ring>(1024, params)};

    if (Scheduler::sharedRingFileDescriptor == -1) Scheduler::sharedRingFileDescriptor = ring->getFileDescriptor();

    const auto result{std::ranges::find(Scheduler::ringFileDescriptors, -1)};
    if (result != Scheduler::ringFileDescriptors.cend()) {
        *result = ring->getFileDescriptor();
    } else
        throw Exception{Log{Log::Level::fatal, "too many Scheduler", sourceLocation}};

    return ring;
}

auto Scheduler::initializeHttpParse() -> HttpParse {
    Database database;
    database.connect({}, "AomaYple", "38820233", "webServer", 0, {}, 0);

    HttpParse httpParse{std::move(database)};

    return httpParse;
}

auto Scheduler::accept(std::source_location sourceLocation) -> Generator {
    this->server.startAccept();

    while (true) {
        const Outcome outcome{co_await this->server.accept()};
        if (outcome.result >= 0 && (outcome.flags & IORING_CQE_F_MORE)) {
            this->clients.emplace(outcome.result, Client{outcome.result, this->ring, 60});
            Client &client{this->clients.at(outcome.result)};

            client.setFirstGenerator(this->receive(client));
            this->timer.add(outcome.result, client.getSeconds());
        } else
            throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
    }
}

auto Scheduler::timing(std::source_location sourceLocation) -> Generator {
    const Outcome outcome{co_await this->timer.timing()};
    if (outcome.result == sizeof(unsigned long))
        for (const int fileDescriptor: this->timer.clearTimeout()) this->clients.erase(fileDescriptor);
    else
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::receive(Client &client, std::source_location sourceLocation) -> Generator {
    client.startReceive();

    while (true) {
        const Outcome outcome{co_await client.receive()};
        if (outcome.result > 0 && (outcome.flags & IORING_CQE_F_MORE)) {
            client.writeToBuffer(client.getReceivedData(outcome.flags >> IORING_CQE_BUFFER_SHIFT, outcome.result));

            if (!(outcome.flags & IORING_CQE_F_SOCK_NONEMPTY)) {
                client.setSecondGenerator(this->send(client));

                this->timer.update(client.getFileDescriptor(), client.getSeconds());
            }
        } else
            throw Exception{Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation}};
    }
}

auto Scheduler::send(Client &client, std::source_location sourceLocation) -> Generator {
    const std::span<const std::byte> receivedData{client.readFromBuffer()};
    std::vector<std::byte> response{this->httpParse.parse(
            std::string_view{reinterpret_cast<const char *>(receivedData.data()), receivedData.size()})};

    client.clearBuffer();
    client.writeToBuffer(response);

    const Outcome outcome{co_await client.send()};
    if (outcome.result > 0) client.clearBuffer();
    else
        throw Exception{Log{Log::Level::warn, std::strerror(std::abs(outcome.result)), sourceLocation}};
}

auto Scheduler::closed(int fileDescriptor, int result, std::source_location sourceLocation) -> void {
    if (result < 0)
        logger::push(Log{Log::Level::warn, std::to_string(fileDescriptor) + ": " + std::strerror(std::abs(result)),
                         sourceLocation});
}

constinit thread_local bool Scheduler::instance{};
constinit std::mutex Scheduler::lock;
constinit int Scheduler::sharedRingFileDescriptor{-1};
std::vector<int> Scheduler::ringFileDescriptors{std::vector<int>(std::thread::hardware_concurrency(), -1)};
constinit std::atomic_flag Scheduler::switcher{true};
