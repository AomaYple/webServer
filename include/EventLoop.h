#pragma once

#include <source_location>

#include "BufferRing.h"
#include "Client.h"
#include "Server.h"

class EventLoop {
public:
    EventLoop();

    EventLoop(const EventLoop &other) = delete;

    EventLoop(EventLoop &&other) = delete;

    [[noreturn]] auto loop() -> void;

    ~EventLoop();

private:
    auto handleAccept(int result, unsigned int flags,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleReceive(int result, int socket, unsigned int flags,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleSend(int result, int socket, unsigned int flags,
                    std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto handleClose(int result, int socket,
                            std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    static auto handleCancel(int result, int socket,
                             std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static std::vector<int> values;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    std::unordered_map<int, Client> clients;
};
