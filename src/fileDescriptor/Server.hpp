#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"
#include "FileDescriptor.hpp"

#include <netinet/in.h>

class Server : public FileDescriptor {
public:
    [[nodiscard]] static auto create(unsigned short port) -> int;

    Server(int fileDescriptor, std::shared_ptr<Ring> ring);

    Server(const Server &) = delete;

    auto operator=(const Server &) -> Server & = delete;

    Server(Server &&) = default;

    auto operator=(Server &&) -> Server & = default;

    ~Server() = default;

    auto setGenerator(Generator &&newGenerator) noexcept -> void;

    auto resumeGenerator(Outcome outcome) -> void;

    auto startAccept() const -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

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

    Generator generator{nullptr};
    Awaiter awaiter{};
};
