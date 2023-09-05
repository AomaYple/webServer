#pragma once

#include "../coroutine/Awaiter.h"

#include <liburing.h>

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Timer {
public:
    static auto create() noexcept -> unsigned int;

    explicit Timer(unsigned int fileDescriptorIndex) noexcept;

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

private:
    [[nodiscard]] static auto createFileDescriptor() noexcept -> unsigned int;

    static auto setTime(unsigned int fileDescriptor) noexcept -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto setResult(std::pair<int, unsigned int> result) noexcept -> void;

    [[nodiscard]] auto timing(io_uring_sqe *sqe) noexcept -> const Awaiter &;

    [[nodiscard]] auto clearTimeout() noexcept -> std::vector<unsigned int>;

    auto add(unsigned int fileDescriptor, unsigned char timeout) noexcept -> void;

    auto update(unsigned int fileDescriptor, unsigned char timeout) noexcept -> void;

    auto remove(unsigned int fileDescriptor) noexcept -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

private:
    const unsigned int fileDescriptorIndex;
    unsigned char now;
    unsigned long expireCount;
    std::array<std::unordered_set<unsigned int>, 61> wheel;
    std::unordered_map<unsigned int, unsigned char> location;
    Awaiter awaiter;
};
