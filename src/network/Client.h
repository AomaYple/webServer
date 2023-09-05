#pragma once

#include "../coroutine/Awaiter.h"

#include <liburing.h>

#include <span>
#include <vector>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned char;

    auto setResult(std::pair<int, unsigned int> result) noexcept -> void;

    auto startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto writeReceivedData(std::span<const std::byte> data) noexcept -> void;

    [[nodiscard]] auto readReceivedData() noexcept -> std::vector<std::byte>;

    [[nodiscard]] auto send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter &;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

private:
    const unsigned int fileDescriptorIndex;
    const unsigned char timeout;
    std::vector<std::byte> receivedData, unSendData;
    Awaiter awaiter;
};
