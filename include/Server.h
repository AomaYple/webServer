#pragma once

#include <memory>

#include "Ring.h"

class Server {
public:
    Server(unsigned short port, const std::shared_ptr<Ring> &ring);

    Server(const Server &other) = delete;

    Server(Server &&other) noexcept;

    auto operator=(Server &&other) noexcept -> Server &;

    ~Server();

private:
    auto setSocketOption() const -> void;

    auto bind(unsigned short port) const -> void;

    auto listen() const -> void;

    auto registerFileDescriptor() -> void;

    int socket;
    std::shared_ptr<Ring> ring;
};
