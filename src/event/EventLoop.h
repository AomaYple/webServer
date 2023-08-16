#pragma once

#include "../base/BufferRing.h"
#include "../http/Database.h"
#include "../socket/Server.h"
#include "Timer.h"

class EventLoop {
public:
    EventLoop();

    EventLoop(const EventLoop &) = delete;

    EventLoop(EventLoop &&) noexcept;

    [[noreturn]] auto loop() -> void;

    ~EventLoop();

private:
    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static std::vector<int> cpus;
    static constexpr unsigned int ringEntries{128};
    static constexpr unsigned int bufferRingEntries{128};
    static constexpr __u16 bufferRingId{0};
    static constexpr std::uint_least16_t port{9999};
    static constexpr std::size_t bufferRingBufferSize{1024};

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
};
