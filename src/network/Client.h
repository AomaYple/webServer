#pragma once

#include "../coroutine/Awaiter.h"
#include "../coroutine/Task.h"

#include <liburing.h>

#include <span>
#include <vector>

class Client {
public:
    Client(uint32_t fileDescriptorIndex, uint8_t timeout) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> uint32_t;

    [[nodiscard]] auto getTimeout() const noexcept -> uint8_t;

    auto startReceive(io_uring_sqe *sqe, uint16_t bufferRingId) const noexcept -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto setReceiveTask(Task &&task) noexcept -> void;

    auto resumeReceive(std::pair<int32_t, uint32_t> result) -> void;

    auto writeData(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter &;

    auto setSendTask(Task &&task) noexcept -> void;

    auto resumeSend(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto readData() noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int32_t, uint32_t> result) -> void;

private:
    const uint32_t fileDescriptorIndex;
    const uint8_t timeout;
    std::vector<std::byte> buffer;
    Task receiveTask, sendTask, cancelTask, closeTask;
    Awaiter awaiter;
};
