#pragma once

#include <arpa/inet.h>

#include <source_location>
#include <string>

class Ring;

class Server {
public:
    Server(unsigned short port, Ring &ring);

    Server(const Server &server) = delete;

    Server(Server &&server) noexcept;

    auto operator=(Server &&server) noexcept -> Server &;

    auto accept(Ring &ring) -> void;

    ~Server();

private:
    static thread_local bool instance;

    int self;
};
