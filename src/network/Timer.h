#pragma once

#include "../coroutine/Awaiter.h"
#include "../coroutine/Task.h"

#include <liburing.h>

#include <array>
#include <source_location>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Timer {
public:
    [[nodiscard]] static auto create() -> uint32_t;

    explicit Timer(uint32_t fileDescriptorIndex);

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

private:
    [[nodiscard]] static auto
    createFileDescriptor(__clockid_t clockId, int flags,
                         std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setTime(int fileDescriptor, int flags, const itimerspec &newTime, itimerspec *oldTime,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> uint32_t;

    [[nodiscard]] auto timing(io_uring_sqe *sqe) noexcept -> const Awaiter &;

    auto setTimingTask(Task &&task) noexcept -> void;

    auto resumeTiming(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto clearTimeout() -> std::vector<uint32_t>;

    auto add(uint32_t fileDescriptor, uint8_t timeout,
             std::source_location sourceLocation = std::source_location::current()) -> void;

    auto update(uint32_t fileDescriptor, uint8_t timeout,
                std::source_location sourceLocation = std::source_location::current()) -> void;

    auto remove(uint32_t fileDescriptor) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int32_t, uint32_t> result) -> void;

private:
    const uint32_t fileDescriptorIndex;
    uint8_t now;
    uint64_t expireCount;
    std::array<std::unordered_set<uint32_t>, 61> wheel;
    std::unordered_map<uint32_t, uint8_t> location;
    Task timingTask, cancelTask, closeTask;
    Awaiter awaiter;
};
