#pragma once

#include "Server.h"
#include "Epoll.h"
#include "Timer.h"

#include <thread>

class EventLoop {
public:
    explicit EventLoop(unsigned short port, bool startThread = false);

    EventLoop(const EventLoop &eventLoop) = delete;

    EventLoop(EventLoop &&eventLoop) noexcept;

    auto operator=(EventLoop &&eventLoop) noexcept -> EventLoop &;
private:
    Server server;
    Epoll epoll, timeEpoll, socketEpoll;
    Timer timer;
    std::unordered_map<int, std::shared_ptr<Client>> table;
    std::jthread work;

    auto handleTimeEvent() -> void;

    auto handleSocketEvent() -> void;

    auto handleServerEvent() -> void;

    auto handleClientEvent(int fileDescriptor, uint32_t event) -> void;

    auto removeClient(std::shared_ptr<Client> &client) -> void;

    auto handleClientReceivableEvent(std::shared_ptr<Client> &client) -> void;

    auto handleClientSendableEvent(std::shared_ptr<Client> &client) -> void;

    auto handleUnknownEvent(std::shared_ptr<Client> &client) -> void;
};
