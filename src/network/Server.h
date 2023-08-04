#pragma once

#include "../base/UserRing.h"

#include <memory>

class Server {
public:
    Server(std::uint_fast16_t port, const std::shared_ptr<UserRing> &userRing);

    Server(const Server &) = delete;

    auto accept() -> void;

    ~Server();

private:
    static auto socket(std::source_location sourceLocation = std::source_location::current()) -> std::int_fast32_t;

    auto setSocketOption(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto bind(std::uint_fast16_t port, std::source_location sourceLocation = std::source_location::current()) const
            -> void;

    auto listen(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto registerFileDescriptor() -> void;

    auto cancel() -> void;

    auto close() -> void;

    auto unregisterFileDescriptor() -> void;

    std::int_fast32_t fileDescriptor;
    std::shared_ptr<UserRing> userRing;
};
