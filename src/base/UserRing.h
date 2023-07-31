#pragma once

#include <liburing.h>

#include <functional>
#include <source_location>
#include <span>

auto getFileDescriptorLimit(std::source_location sourceLocation = std::source_location::current()) -> unsigned int;

class UserRing {
public:
    UserRing(unsigned int entries, io_uring_params &params,
             std::source_location sourceLocation = std::source_location::current());

    UserRing(const UserRing &) = delete;

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto registerCpu(unsigned short cpuCode, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto registerFileDescriptors(unsigned int fileDescriptorCount,
                                 std::source_location sourceLocation = std::source_location::current()) -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length,
                                     std::source_location sourceLocation = std::source_location::current()) -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<int> fileDescriptors,
                               std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto setupBufferRing(unsigned short entries, unsigned short id,
                                       std::source_location sourceLocation = std::source_location::current())
            -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

    auto submitWait(unsigned int count, std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task) -> unsigned int;

    [[nodiscard]] auto getSqe(std::source_location sourceLocation = std::source_location::current()) -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                           unsigned short bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring userRing;
};