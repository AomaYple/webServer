#pragma once

#include "../base/BufferRing.h"
#include "../database/Database.h"
#include "../network/Server.h"
#include "../network/Timer.h"

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
    static constexpr unsigned int ringEntries{128}, bufferRingBufferSize{1024};
    static constexpr unsigned short bufferRingEntries{128}, bufferRingId{0}, port{9999};

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
};
