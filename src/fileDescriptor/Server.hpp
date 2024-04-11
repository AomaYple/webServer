#pragma once

#include "FileDescriptor.hpp"

#include <netinet/in.h>

#include <source_location>

class Server : public FileDescriptor {
public:
    [[nodiscard]] static auto create(unsigned short port) -> int;

    explicit Server(int fileDescriptor);

    auto startAccept() noexcept -> void;

    [[nodiscard]] auto accept() noexcept -> Awaiter &;

private:
    [[nodiscard]] static auto socket(std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setSocketOption(int fileDescriptor,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto translateIpAddress(in_addr &address,
                                   std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(int fileDescriptor, const sockaddr_in &address,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
        -> void;
};
