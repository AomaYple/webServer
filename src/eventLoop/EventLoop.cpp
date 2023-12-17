#include "EventLoop.hpp"

#include "../fileDescriptor/Client.hpp"
#include "../log/Exception.hpp"
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
    this->closeAll();

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
        this->ring->traverseCompletion([this](const Completion &completion) { this->frame(completion); });
    }

    this->cancelAll();
}

auto EventLoop::initializeRing(std::source_location sourceLocation) -> std::shared_ptr<Ring> {
    if (EventLoop::instance)
        throw Exception{Log{Log::Level::fatal, "one thread can only have one EventLoop", sourceLocation}};
    EventLoop::instance = true;

    io_uring_params params{};
    params.flags = IORING_SETUP_SUBMIT_ALL | IORING_SETUP_CLAMP | IORING_SETUP_COOP_TASKRUN |
                   IORING_SETUP_TASKRUN_FLAG | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;


    const std::lock_guard lockGuard{EventLoop::lock};

    if (EventLoop::sharedRingFileDescriptor != -1) {
        params.wq_fd = EventLoop::sharedRingFileDescriptor;
        params.flags |= IORING_SETUP_ATTACH_WQ;
    }

    const std::shared_ptr<Ring> ring{std::make_shared<Ring>(1024, params)};

    if (EventLoop::sharedRingFileDescriptor == -1) EventLoop::sharedRingFileDescriptor = ring->getFileDescriptor();

    const auto result{std::ranges::find(EventLoop::ringFileDescriptors, -1)};
    if (result != EventLoop::ringFileDescriptors.cend()) {
        *result = ring->getFileDescriptor();
    } else
        throw Exception{Log{Log::Level::fatal, "too many EventLoop", sourceLocation}};

    return ring;
}

auto EventLoop::initializeHttpParse() -> HttpParse {
    Database database;
    database.connect({}, "AomaYple", "38820233", "webServer", 0, {}, 0);

    HttpParse httpParse{std::move(database)};

    return httpParse;
}

auto EventLoop::frame(const Completion &completion) -> void {
    const int result{completion.result}, fileDescriptor{completion.event.fileDescriptor};
    const unsigned int flags{completion.flags};

    switch (completion.event.type) {
        case Event::Type::accept:
            if (result >= 0 || std::abs(result) != ECANCELED) this->accepted(result, flags);
            break;
        case Event::Type::timing:
            if (result >= 0 || std::abs(result) != ECANCELED) this->timed(result);
            break;
        case Event::Type::receive:
            if (result >= 0 || std::abs(result) != ECANCELED) this->received(fileDescriptor, result, flags);
            break;
        case Event::Type::send:
            if (!(flags & IORING_CQE_F_NOTIF) || (result < 0 && std::abs(result) != ECANCELED))
                this->sent(fileDescriptor, result);
            break;
        case Event::Type::cancel:
            this->canceled(fileDescriptor, result);
            break;
        case Event::Type::close:
            this->closed(fileDescriptor, result);
            break;
    }
}

auto EventLoop::accepted(int result, unsigned int flags, std::source_location sourceLocation) -> void {
    if ((flags & IORING_CQE_F_MORE) && result >= 0) {
        Client client{result, this->ring, RingBuffer{1, 1024, result, this->ring}, std::chrono::seconds{60}};

        client.receive();
        this->timer.add(result, client.getSeconds());
        this->clients.emplace(result, std::move(client));
    } else
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result)), sourceLocation}};
}

auto EventLoop::timed(int result, std::source_location sourceLocation) -> void {
    if (result == sizeof(unsigned long)) {
        for (const int fileDescriptor: this->timer.clearTimeout()) this->clients.at(fileDescriptor).cancel();

        this->timer.timing();
    } else
        throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result)), sourceLocation}};
}

auto EventLoop::received(int fileDescriptor, int result, unsigned int flags, std::source_location sourceLocation)
        -> void {
    Client &client{this->clients.at(fileDescriptor)};

    if ((flags & IORING_CQE_F_MORE) && result > 0) {
        std::vector<std::byte> &buffer{client.getBuffer()};

        const std::vector<std::byte> request{client.getReceivedData(flags >> IORING_CQE_BUFFER_SHIFT, result)};
        buffer.insert(buffer.cend(), request.begin(), request.end());

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
            buffer.emplace_back(std::byte{'\0'});
            std::vector<std::byte> response{this->httpParse.parse(
                    std::string{reinterpret_cast<const char *>(buffer.data()), buffer.size() - 1})};

            buffer.clear();
            buffer = std::move(response);
            client.send();

            this->timer.update(fileDescriptor, client.getSeconds());
        }
    } else {
        client.cancel();
        this->timer.remove(fileDescriptor);

        logger::push(Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation});
    }
}

auto EventLoop::sent(int fileDescriptor, int result, std::source_location sourceLocation) -> void {
    Client &client{this->clients.at(fileDescriptor)};

    if (result < 0) {
        client.cancel();
        this->timer.remove(fileDescriptor);

        logger::push(Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation});
    } else
        client.getBuffer().clear();
}

auto EventLoop::canceled(int fileDescriptor, int result, std::source_location sourceLocation) const -> void {
    if (result < 0) logger::push(Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation});

    if (fileDescriptor == this->server.getFileDescriptor()) this->server.close();
    else if (fileDescriptor == this->timer.getFileDescriptor())
        this->timer.close();
    else
        this->clients.at(fileDescriptor).close();
}

auto EventLoop::closed(int fileDescriptor, int result, std::source_location sourceLocation) -> void {
    if (result < 0) logger::push(Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation});

    this->clients.erase(fileDescriptor);
}

auto EventLoop::cancelAll() -> void {
    this->server.cancel();
    this->timer.cancel();
    for (const auto &client: this->clients) client.second.cancel();

    this->ring->wait(4 + this->clients.size() * 2);
    this->ring->traverseCompletion([this](const Completion &completion) { this->frame(completion); });
}

auto EventLoop::closeAll() -> void {
    this->server.close();
    this->timer.close();
    for (const auto &client: this->clients) client.second.close();

    this->ring->wait(2 + this->clients.size());
    this->ring->traverseCompletion([this](const Completion &completion) { this->frame(completion); });
}

constinit thread_local bool EventLoop::instance{false};
constinit std::mutex EventLoop::lock;
constinit int EventLoop::sharedRingFileDescriptor{-1};
std::vector<int> EventLoop::ringFileDescriptors{std::vector<int>(std::thread::hardware_concurrency(), -1)};
constinit std::atomic_flag EventLoop::switcher{true};
