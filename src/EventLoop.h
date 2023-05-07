#pragma once

#include "Epoll.h"
#include "Server.h"
#include "Timer.h"

#include <thread>

class EventLoop {
public:
    explicit EventLoop(unsigned short port, bool startThread = false);

    EventLoop(const EventLoop &eventLoop) = delete;

    EventLoop(EventLoop &&eventLoop) noexcept;

    auto operator=(EventLoop &&eventLoop) noexcept -> EventLoop &;
private:
    auto handleServerEvent() -> void;

    auto handleClientEvent(int fileDescriptor, uint32_t event, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleClientReceivableEvent(const std::shared_ptr<Client> &client) -> void;

    auto handleClientSendableEvent(const std::shared_ptr<Client> &client) -> void;

    Server server;
    Timer timer;
    Epoll epoll;
    std::jthread work;
};
