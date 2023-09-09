#pragma once

#include "../coroutine/Awaiter.h"

#include <liburing.h>

#include <array>
#include <source_location>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Timer {
public:
    static auto create() -> unsigned int;

    explicit Timer(unsigned int fileDescriptorIndex);

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

private:
    [[nodiscard]] static auto
    createFileDescriptor(__clockid_t clockId, int flags,
                         std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setTime(int fileDescriptor, int flags, const itimerspec &newTime, itimerspec *oldTime,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto timing(io_uring_sqe *sqe) noexcept -> const Awaiter &;

    [[nodiscard]] auto clearTimeout() -> std::vector<unsigned int>;

    auto add(unsigned int fileDescriptor, unsigned char timeout,
             std::source_location sourceLocation = std::source_location::current()) -> void;

    auto update(unsigned int fileDescriptor, unsigned char timeout,
                std::source_location sourceLocation = std::source_location::current()) -> void;

    auto remove(unsigned int fileDescriptor) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setResult(std::pair<int, unsigned int> result) noexcept -> void;

private:
    const unsigned int fileDescriptorIndex;
    unsigned char now;
    unsigned long expireCount;
    std::array<std::unordered_set<unsigned int>, 61> wheel;
    std::unordered_map<unsigned int, unsigned char> location;
    Awaiter awaiter;
};
