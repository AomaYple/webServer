#pragma once

#include "../coroutine/Awaiter.h"

#include <liburing.h>

class Server {
public:
    [[nodiscard]] static auto create(unsigned short port) noexcept -> unsigned int;

    explicit Server(unsigned int fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

private:
    [[nodiscard]] static auto socket() noexcept -> unsigned int;

    static auto setSocketOption(unsigned int fileDescriptor) noexcept -> void;

    static auto bind(unsigned int fileDescriptor, unsigned short port) noexcept -> void;

    static auto listen(unsigned int fileDescriptor) noexcept -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto setResult(std::pair<int, unsigned int> result) noexcept -> void;

    auto startAccept(io_uring_sqe *sqe) const noexcept -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

private:
    const unsigned int fileDescriptorIndex;
    Awaiter awaiter;
};
