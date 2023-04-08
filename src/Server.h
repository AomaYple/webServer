#pragma once

#include "Client.h"

#include <memory>

#include <arpa/inet.h>

class Server {
public:
    explicit Server(unsigned short port, std::source_location sourceLocation = std::source_location::current());

    Server(const Server &server) = delete;

    Server(Server &&server) noexcept;

    auto operator=(Server &&server) noexcept -> Server &;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> std::vector<std::shared_ptr<Client>>;

    [[nodiscard]] auto get() const -> int;

    ~Server();
private:
    int self, idleFileDescriptor;
    sockaddr_in address;
    socklen_t addressLength;
};
