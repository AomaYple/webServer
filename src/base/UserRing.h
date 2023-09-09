#pragma once

#include <liburing.h>
#include <sys/resource.h>

#include <functional>
#include <source_location>
#include <span>

class UserRing {
public:
    static auto getFileDescriptorLimit(std::source_location sourceLocation = std::source_location::current()) -> rlim_t;

    UserRing(unsigned int entries, io_uring_params &params,
             std::source_location sourceLocation = std::source_location::current());

    UserRing(const UserRing &) = delete;

    UserRing(UserRing &&) noexcept;

    auto getSelfFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto registerCpu(unsigned short cpuCode, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto registerSparseFileDescriptors(unsigned int fileDescriptorCount,
                                       std::source_location sourceLocation = std::source_location::current()) -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length,
                                     std::source_location sourceLocation = std::source_location::current()) -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<const int> fileDescriptors,
                               std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto setupBufferRing(unsigned short entries, int id,
                                       std::source_location sourceLocation = std::source_location::current())
            -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, int id,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

    auto submitWait(unsigned int count, std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task) noexcept
            -> unsigned int;

    [[nodiscard]] auto getSqe(std::source_location sourceLocation = std::source_location::current()) -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, int completionCount,
                                           int bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring userRing;
};
