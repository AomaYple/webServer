#pragma once

#include <memory>

#include "../base/UserRing.h"

class Server {
public:
    Server(unsigned short port, const std::shared_ptr<UserRing> &userRing);

    Server(const Server &) = delete;

    auto accept() -> void;

    ~Server();

private:
    static auto socket(std::source_location sourceLocation = std::source_location::current()) -> int;

    auto setSocketOption(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto bind(unsigned short port, std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto listen(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto registerFileDescriptor() -> void;

    auto cancel() -> void;

    auto close() -> void;

    auto unregisterFileDescriptor() -> void;

    int fileDescriptor;
    std::shared_ptr<UserRing> userRing;
};
