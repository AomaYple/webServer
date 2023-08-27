#pragma once

#include "../base/UserRing.h"

#include <memory>

class Server {
public:
    static auto create(unsigned short port) -> unsigned int;

    Server(unsigned int fileDescriptorIndex, const std::shared_ptr<UserRing> &userRing);

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

private:
    static auto socket(std::source_location sourceLocation = std::source_location::current()) -> unsigned int;

    static auto setSocketOption(unsigned int fileDescriptor,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(unsigned int fileDescriptor, unsigned short port,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(unsigned int fileDescriptor,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    auto accept() const -> void;

    ~Server();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    const unsigned int fileDescriptorIndex;
    std::shared_ptr<UserRing> userRing;
};
