#pragma once

#include "../coroutine/Awaiter.h"
#include "../coroutine/Task.h"

#include <liburing.h>

#include <span>
#include <vector>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned char;

    auto startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto setReceiveTask(Task &&task) noexcept -> void;

    auto resumeReceive(std::pair<int, unsigned int> result) -> void;

    auto writeData(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter &;

    auto setSendTask(Task &&task) noexcept -> void;

    auto resumeSend(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto readData() noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int, unsigned int> result) -> void;

private:
    const unsigned int fileDescriptorIndex;
    const unsigned char timeout;
    std::vector<std::byte> buffer;
    Task receiveTask, sendTask, cancelTask, closeTask;
    Awaiter awaiter;
};
