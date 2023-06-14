#pragma once

#include "Epoll.h"
#include "Server.h"
#include "Timer.h"

class EventLoop {
public:
    explicit EventLoop(unsigned short port);

    EventLoop(const EventLoop &other) = delete;

    EventLoop(EventLoop &&other) noexcept;

    auto operator=(EventLoop &&other) noexcept -> EventLoop &;

    auto operator()() -> void;

private:
    auto handleServer() -> void;

    auto handleClient(int socket, unsigned int event,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleClientReceive(std::shared_ptr<Client> &client) -> void;

    auto handleClientSend(std::shared_ptr<Client> &client) -> void;

    Server server;
    Timer timer;
    Epoll epoll;
};
