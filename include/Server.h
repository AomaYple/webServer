#pragma once

#include <memory>

#include "Client.h"

class Server {
public:
    explicit Server(unsigned short port);

    Server(const Server &other) = delete;

    Server(Server &&other) noexcept;

    auto operator=(Server &&other) noexcept -> Server &;

    [[nodiscard]] auto get() const -> int;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::shared_ptr<Client>>;

    ~Server();

private:
    auto setSocketOption() const -> void;

    auto bind(unsigned short port) const -> void;

    auto listen() const -> void;

    int socket, idleFileDescriptor;
};