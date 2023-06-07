#pragma once

#include <arpa/inet.h>

#include <memory>

class Ring;

class Server {
public:
    Server(unsigned short port, std::shared_ptr<Ring> &ring);

    Server(const Server &server) = delete;

    Server(Server &&server) noexcept;

    auto operator=(Server &&server) noexcept -> Server &;

    auto accept(std::shared_ptr<Ring> &ring) const -> void;

    ~Server();

private:
    static thread_local bool instance;

    int self;
};
