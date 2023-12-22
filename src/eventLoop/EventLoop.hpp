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
    [[nodiscard]] static auto initializeRing(std::source_location sourceLocation = std::source_location::current())
            -> std::shared_ptr<Ring>;

    [[nodiscard]] static auto initializeHttpParse() -> HttpParse;

    auto accepted(int result, unsigned int flags, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto timed(int result, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto received(int fileDescriptor, int result, unsigned int flags,
                  std::source_location sourceLocation = std::source_location::current()) -> void;

    auto sent(int fileDescriptor, int result, unsigned int flags,
              std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto closed(int fileDescriptor, int result,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedRingFileDescriptor;
    static std::vector<int> ringFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<Ring> ring{EventLoop::initializeRing()};
    const Server server{0, this->ring};
    Timer timer{1, this->ring};
    HttpParse httpParse{EventLoop::initializeHttpParse()};
    std::unordered_map<int, Client> clients;
};
