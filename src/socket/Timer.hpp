#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <liburing.h>

#include <array>
#include <source_location>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Timer {
public:
    explicit Timer(unsigned int fileDescriptorIndex);

    Timer(const Timer &) = delete;

    Timer(Timer &&) = default;

    auto operator=(const Timer &) -> Timer & = delete;

    auto operator=(Timer &&) -> Timer & = delete;

    ~Timer() = default;

    [[nodiscard]] static auto create() -> unsigned int;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto timing(io_uring_sqe *sqe) noexcept -> const Awaiter &;

    auto setTimingGenerator(Generator &&generator) noexcept -> void;

    auto resumeTiming(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto clearTimeout() -> std::vector<unsigned int>;

    auto add(unsigned int fileDescriptor, unsigned char timeout,
             std::source_location sourceLocation = std::source_location::current()) -> void;

    auto update(unsigned int fileDescriptor, unsigned char timeout,
                std::source_location sourceLocation = std::source_location::current()) -> void;

    auto remove(unsigned int fileDescriptor) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    auto resumeCancel(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    auto resumeClose(std::pair<int, unsigned int> result) -> void;

private:
    [[nodiscard]] static auto
    createFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> unsigned int;

    static auto setTime(unsigned int fileDescriptor,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

    const unsigned int fileDescriptorIndex;
    unsigned char now;
    unsigned long expireCount;
    std::array<std::unordered_set<unsigned int>, 61> wheel;
    std::unordered_map<unsigned int, unsigned char> location;
    Generator timingGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
