#pragma once

#include "../coroutine/Awaiter.h"

#include <liburing.h>

#include <span>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto getTimeout() const noexcept -> unsigned char;

    auto startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void;

    auto receive() const noexcept -> const Awaiter &;

    [[nodiscard]] auto send(io_uring_sqe *sqe, std::span<const std::byte> unsSendData) noexcept -> const Awaiter &;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setResult(std::pair<int, unsigned int> result) noexcept -> void;

private:
    const unsigned int fileDescriptorIndex;
    const unsigned char timeout;
    Awaiter awaiter;
};
