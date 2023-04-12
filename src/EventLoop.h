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

    auto handleServerEvent() -> void;

    auto handleClientEvent(int fileDescriptor, uint32_t event) -> void;

    auto handleClientReceivableEvent(const std::shared_ptr<Client> &client) -> void;

    auto handleClientSendableEvent(const std::shared_ptr<Client> &client) -> void;
private:
    Server server;
    Timer timer;
    Epoll epoll;
    std::jthread work;
};
