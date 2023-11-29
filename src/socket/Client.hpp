#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <liburing.h>

#include <span>
#include <vector>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned short timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) = default;

    auto operator=(const Client &) -> Client & = delete;

    auto operator=(Client &&) -> Client & = delete;

    ~Client() = default;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned short;

    auto writeToBuffer(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto readFromBuffer() const -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    auto startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto setReceiveGenerator(Generator &&generator) noexcept -> void;

    auto resumeReceive(Result result) -> void;

    [[nodiscard]] auto send(io_uring_sqe *sqe) noexcept -> const Awaiter &;

    auto setSendGenerator(Generator &&generator) noexcept -> void;

    auto resumeSend(Result result) -> void;

    auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    auto resumeCancel(Result result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    auto resumeClose(Result result) -> void;

private:
    const unsigned int fileDescriptorIndex;
    const unsigned short timeout;
    std::vector<std::byte> buffer;
    Generator receiveGenerator, sendGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
