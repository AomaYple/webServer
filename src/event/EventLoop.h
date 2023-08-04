#pragma once

#include "../base/BufferRing.h"
#include "../network/Server.h"
#include "Timer.h"

class EventLoop {
public:
    EventLoop();

    EventLoop(const EventLoop &) = delete;

    [[noreturn]] auto loop() -> void;

    ~EventLoop();

private:
    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static std::vector<std::int_fast32_t> cpus;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
};
