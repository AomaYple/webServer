#pragma once

#include <source_location>

#include "BufferRing.h"
#include "Client.h"
#include "Server.h"

class EventLoop {
public:
    explicit EventLoop(unsigned short port);

    EventLoop(const EventLoop &other) = delete;

    EventLoop(EventLoop &&other) noexcept;

    auto operator=(EventLoop &&other) noexcept -> EventLoop &;

    auto loop() -> void;

private:
    auto handleAccept(int result, unsigned int flags,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleReceive(int result, int fileDescriptor, unsigned int flags,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    std::shared_ptr<Ring> ring;
    BufferRing bufferRing;
    Server server;
    std::unordered_map<int, Client> clients;
};
