#pragma once

#include <liburing.h>

#include <cstdint>
#include <functional>
#include <source_location>
#include <span>

auto getFileDescriptorLimit(std::source_location sourceLocation = std::source_location::current())
        -> std::uint_fast64_t;

class UserRing {
public:
    UserRing(std::uint_fast32_t entries, io_uring_params &params,
             std::source_location sourceLocation = std::source_location::current());

    UserRing(const UserRing &) = delete;

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> std::int_fast32_t;

    auto registerSelfFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto registerCpu(std::uint_fast16_t cpuCode, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto registerFileDescriptors(std::uint_fast32_t fileDescriptorCount,
                                 std::source_location sourceLocation = std::source_location::current()) -> void;

    auto allocateFileDescriptorRange(std::uint_fast32_t offset, std::uint_fast32_t length,
                                     std::source_location sourceLocation = std::source_location::current()) -> void;

    auto updateFileDescriptors(std::uint_fast32_t offset, std::span<std::int_fast32_t> fileDescriptors,
                               std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto setupBufferRing(std::uint_fast16_t entries, std::uint_fast16_t id,
                                       std::source_location sourceLocation = std::source_location::current())
            -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, std::uint_fast16_t entries, std::uint_fast16_t id,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

    auto submitWait(std::uint_fast32_t count, std::source_location sourceLocation = std::source_location::current())
            -> void;

    [[nodiscard]] auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task)
            -> std::uint_fast32_t;

    [[nodiscard]] auto getSqe(std::source_location sourceLocation = std::source_location::current()) -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, std::uint_fast32_t completionCount,
                                           std::uint_fast16_t bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring userRing;
};
