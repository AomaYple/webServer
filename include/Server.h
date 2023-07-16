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
    auto setSocketOption() const -> void;

    auto bind(unsigned short port) const -> void;

    auto listen() const -> void;

    auto registerSelf() -> void;

    auto cancel() -> void;

    auto close() -> void;

    auto unregisterSelf() -> void;

    int socket;
    std::shared_ptr<UserRing> userRing;
};
