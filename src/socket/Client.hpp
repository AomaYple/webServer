#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <liburing.h>

#include <span>
#include <vector>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) = default;

    auto operator=(const Client &) -> Client & = delete;

    auto operator=(Client &&) -> Client & = default;

    ~Client() = default;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned char;

    auto startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto setReceiveGenerator(Generator &&generator) noexcept -> void;

    auto resumeReceive(std::pair<int, unsigned int> result) -> void;

    auto writeData(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter &;

    auto setSendGenerator(Generator &&generator) noexcept -> void;

    auto resumeSend(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto readData() noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    auto resumeCancel(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    auto resumeClose(std::pair<int, unsigned int> result) -> void;

private:
    unsigned int fileDescriptorIndex;
    unsigned char timeout;
    std::vector<std::byte> buffer;
    Generator receiveGenerator, sendGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
