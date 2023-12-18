#pragma once

#include "FileDescriptor.hpp"

#include <netinet/in.h>

class Server : public FileDescriptor {
public:
    [[nodiscard]] static auto create(unsigned short port) -> int;

    Server(int fileDescriptor, std::shared_ptr<Ring> ring);

    Server(const Server &) = delete;

    auto operator=(const Server &) -> Server & = delete;

    Server(Server &&) noexcept = default;

    auto operator=(Server &&) noexcept -> Server & = default;

    ~Server() = default;

    auto accept() const -> void;

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
