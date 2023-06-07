#pragma once

#include "Buffer.h"
#include "Server.h"

class EventLoop {
public:
    explicit EventLoop(unsigned short port);

    EventLoop(const EventLoop &eventLoop) = delete;

    EventLoop(EventLoop &&eventLoop) = delete;

    auto loop() -> void;

private:
    std::shared_ptr<Ring> ring;
    Buffer buffer;
    Server server;
};
