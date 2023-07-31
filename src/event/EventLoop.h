#pragma once

#include <source_location>

#include "../base/BufferRing.h"
#include "../network/Server.h"
#include "../timer/Timer.h"

class EventLoop {
public:
    EventLoop();

    EventLoop(const EventLoop &) = delete;

    [[noreturn]] auto loop() -> void;

    ~EventLoop();

private:
    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static std::vector<int> cpus;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
};
