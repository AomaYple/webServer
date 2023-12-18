#pragma once

#include "../fileDescriptor/Server.hpp"
#include "../fileDescriptor/Timer.hpp"
#include "../http/HttpParse.hpp"

class Client;

class EventLoop {
public:
    static auto registerSignal(std::source_location sourceLocation = std::source_location::current()) -> void;

    EventLoop();

    EventLoop(const EventLoop &) = delete;

    auto operator=(const EventLoop &) -> EventLoop & = delete;

    EventLoop(EventLoop &&) = delete;

    auto operator=(EventLoop &&) -> EventLoop & = delete;

    ~EventLoop();

    auto run() -> void;

private:
    auto frame(const Completion &completion) -> void;

    auto accepted(int result, unsigned int flags, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto timed(int result, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto received(int fileDescriptor, int result, unsigned int flags,
                  std::source_location sourceLocation = std::source_location::current()) -> void;

    auto sent(int fileDescriptor, int result, std::source_location sourceLocation = std::source_location::current())
            -> void;

    static auto canceled(int fileDescriptor, int result,
                         std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto closed(int fileDescriptor, int result,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedRingFileDescriptor;
    static std::vector<int> ringFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<Ring> ring{[](std::source_location sourceLocation = std::source_location::current()) {
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

        const std::shared_ptr<Ring> temporaryRing{std::make_shared<Ring>(1024, params)};

        if (EventLoop::sharedRingFileDescriptor == -1)
            EventLoop::sharedRingFileDescriptor = temporaryRing->getFileDescriptor();

        const auto result{std::ranges::find(EventLoop::ringFileDescriptors, -1)};
        if (result != EventLoop::ringFileDescriptors.cend()) {
            *result = temporaryRing->getFileDescriptor();
        } else
            throw Exception{Log{Log::Level::fatal, "too many EventLoop", sourceLocation}};

        return temporaryRing;
    }()};
    const Server server{0, this->ring};
    Timer timer{1, this->ring};
    HttpParse httpParse{[] {
        Database database;
        database.connect({}, "AomaYple", "38820233", "webServer", 0, {}, 0);

        HttpParse temporaryHttpParse{std::move(database)};

        return temporaryHttpParse;
    }()};
    std::unordered_map<int, Client> clients;
};
