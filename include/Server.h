#pragma once

#include <memory>

#include "UserRing.h"

class Server {
public:
    Server(unsigned short port, const std::shared_ptr<UserRing> &userRing);

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

    auto operator=(Server &&) noexcept -> Server &;

    auto accept() -> void;

    ~Server();

private:
    auto socket() -> void;

    auto setSocketOption() const -> void;

    auto bind(unsigned short port) const -> void;

    auto listen() const -> void;

    auto registerFileDescriptor() -> void;

    auto cancel() -> void;

    auto close() -> void;

    auto unregisterFileDescriptor() -> void;

    int fileDescriptor;
    std::shared_ptr<UserRing> userRing;
};
