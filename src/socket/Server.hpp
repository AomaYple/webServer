#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <liburing.h>

#include <netinet/in.h>
#include <source_location>

class Server {
public:
    explicit Server(unsigned int fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    Server(Server &&) = default;

    auto operator=(const Server &) -> Server & = delete;

    auto operator=(Server &&) -> Server & = delete;

    ~Server() = default;

    [[nodiscard]] static auto create(unsigned short port) -> unsigned int;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto startAccept(io_uring_sqe *sqe) const noexcept -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    auto setAcceptGenerator(Generator &&generator) noexcept -> void;

    auto resumeAccept(Result result) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    auto resumeCancel(Result result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    auto resumeClose(Result result) -> void;

private:
    [[nodiscard]] static auto socket(std::source_location sourceLocation = std::source_location::current())
            -> unsigned int;

    static auto setSocketOption(unsigned int fileDescriptor,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto translateIpAddress(in_addr &address,
                                   std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(unsigned int fileDescriptor, const sockaddr_in &address,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(unsigned int fileDescriptor,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    const unsigned int fileDescriptorIndex;
    Generator acceptGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
