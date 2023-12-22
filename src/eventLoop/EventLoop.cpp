#include "EventLoop.hpp"

#include "../fileDescriptor/Client.hpp"
#include "../log/logger.hpp"
#include "../ring/Completion.hpp"

#include <cstring>

auto EventLoop::registerSignal(std::source_location sourceLocation) -> void {
    struct sigaction signalAction {};
    signalAction.sa_handler = [](int) noexcept { EventLoop::switcher.clear(std::memory_order_relaxed); };

    if (sigaction(SIGTERM, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    if (sigaction(SIGINT, &signalAction, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

EventLoop::EventLoop() {
    this->ring->registerSelfFileDescriptor();

    {
        const std::lock_guard lockGuard{EventLoop::lock};

        const auto result{std::ranges::find(EventLoop::ringFileDescriptors, this->ring->getFileDescriptor())};
        this->ring->registerCpu(std::distance(EventLoop::ringFileDescriptors.begin(), result));
    }

    this->ring->registerSparseFileDescriptor(Ring::getFileDescriptorLimit());
    this->ring->allocateFileDescriptorRange(2, Ring::getFileDescriptorLimit() - 2);

    const std::array<int, 2> fileDescriptors{Server::create(8080), Timer::create()};
    this->ring->updateFileDescriptors(0, fileDescriptors);
}

EventLoop::~EventLoop() {
    {
        const std::lock_guard lockGuard{EventLoop::lock};

        auto result{std::ranges::find(EventLoop::ringFileDescriptors, this->ring->getFileDescriptor())};
        *result = -1;

        if (EventLoop::sharedRingFileDescriptor == this->ring->getFileDescriptor()) {
            EventLoop::sharedRingFileDescriptor = -1;

            result = std::ranges::find_if(EventLoop::ringFileDescriptors,
                                          [](const int fileDescriptor) noexcept { return fileDescriptor != -1; });
            if (result != EventLoop::ringFileDescriptors.cend()) EventLoop::sharedRingFileDescriptor = *result;
        }
    }

    EventLoop::instance = false;
}

auto EventLoop::run() -> void {
    this->server.accept();
    this->timer.timing();

    while (EventLoop::switcher.test(std::memory_order_relaxed)) {
        this->ring->wait(1);
        this->ring->traverseCompletion([this](const Completion &completion) {
            const int result{completion.result}, fileDescriptor{completion.event.fileDescriptor};
            const unsigned int flags{completion.flags};

            switch (completion.event.type) {
                case Event::Type::accept:
                    this->accepted(result, flags);
                    break;
                case Event::Type::timing:
                    this->timed(result);
                    break;
                case Event::Type::receive:
                    this->received(fileDescriptor, result, flags);
                    break;
                case Event::Type::send:
                    this->sent(fileDescriptor, result, flags);
                    break;
                case Event::Type::close:
                    EventLoop::closed(fileDescriptor, result);
                    break;
            }
        });
    }
}

auto EventLoop::initializeRing(std::source_location sourceLocation) -> std::shared_ptr<Ring> {
    if (EventLoop::instance)
        throw Exception{Log{Log::Level::fatal, "one thread can only have one EventLoop", sourceLocation}};
    EventLoop::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;


    std::shared_ptr<Ring> ring;
    {
        const std::lock_guard lockGuard{EventLoop::lock};

        if (EventLoop::sharedRingFileDescriptor != -1) {
            params.wq_fd = EventLoop::sharedRingFileDescriptor;
            params.flags |= IORING_SETUP_ATTACH_WQ;
        }
    }
    ring = std::make_shared<Ring>(1024, params);
    {
        const std::lock_guard lockGuard{EventLoop::lock};

        if (EventLoop::sharedRingFileDescriptor == -1) EventLoop::sharedRingFileDescriptor = ring->getFileDescriptor();

        const auto result{std::ranges::find(EventLoop::ringFileDescriptors, -1)};
        if (result != EventLoop::ringFileDescriptors.cend()) {
            *result = ring->getFileDescriptor();
        } else
            throw Exception{Log{Log::Level::fatal, "too many EventLoop", sourceLocation}};
    }

    return ring;
}

auto EventLoop::initializeHttpParse() -> HttpParse {
    Database database;
    database.connect({}, "AomaYple", "38820233", "webServer", 0, {}, 0);

    HttpParse httpParse{std::move(database)};

    return httpParse;
}

auto EventLoop::accepted(int result, unsigned int flags, std::source_location sourceLocation) -> void {
    if (result >= 0 && (flags & IORING_CQE_F_MORE)) {
        Client client{result, this->ring, 60};
        client.receive();

        this->timer.add(result, client.getSeconds());
        this->clients.emplace(result, std::move(client));
    } else
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto EventLoop::timed(int result, std::source_location sourceLocation) -> void {
    if (result == sizeof(unsigned long)) {
        for (const int fileDescriptor: this->timer.clearTimeout()) this->clients.erase(fileDescriptor);

        this->timer.timing();
    } else
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto EventLoop::received(int fileDescriptor, int result, unsigned int flags, std::source_location sourceLocation)
        -> void {
    if (result > 0 && (flags & IORING_CQE_F_MORE)) {
        Client &client{this->clients.at(fileDescriptor)};

        std::vector<std::byte> receivedData{client.getReceivedData(flags >> IORING_CQE_BUFFER_SHIFT, result)};

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
            receivedData.emplace_back(std::byte{'\0'});

            std::vector<std::byte> response{this->httpParse.parse(
                    std::string_view{reinterpret_cast<const char *>(receivedData.data()), receivedData.size() - 1})};
            client.send(std::move(response));

            this->timer.update(fileDescriptor, client.getSeconds());
        }
    } else {
        this->timer.remove(fileDescriptor);
        this->clients.erase(fileDescriptor);

        logger::push(Log{Log::Level::warn, std::strerror(std::abs(result)), sourceLocation});
    }
}

auto EventLoop::sent(int fileDescriptor, int result, unsigned int flags, std::source_location sourceLocation) -> void {
    if (flags & IORING_CQE_F_NOTIF) return;

    if (result <= 0) {
        this->timer.remove(fileDescriptor);
        this->clients.erase(fileDescriptor);

        logger::push(Log{Log::Level::warn, std::strerror(std::abs(result)), sourceLocation});
    } else
        this->clients.at(fileDescriptor).clearSentData();
}

auto EventLoop::closed(int fileDescriptor, int result, std::source_location sourceLocation) -> void {
    if (result < 0)
        logger::push(Log{Log::Level::warn, std::to_string(fileDescriptor) + ": " + std::strerror(std::abs(result)),
                         sourceLocation});
}

constinit thread_local bool EventLoop::instance{false};
constinit std::mutex EventLoop::lock;
constinit int EventLoop::sharedRingFileDescriptor{-1};
std::vector<int> EventLoop::ringFileDescriptors{std::vector<int>(std::thread::hardware_concurrency(), -1)};
constinit std::atomic_flag EventLoop::switcher{true};
