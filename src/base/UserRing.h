#pragma once

#include <liburing.h>

#include <functional>
#include <span>

class UserRing {
public:
    [[nodiscard]] static auto getFileDescriptorLimit() noexcept -> unsigned int;

    UserRing(unsigned int entries, io_uring_params &params) noexcept;

    UserRing(const UserRing &) = delete;

    UserRing(UserRing &&) noexcept;

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> unsigned int;

    auto registerSelfFileDescriptor() noexcept -> void;

    auto registerCpu(unsigned short cpuCode) noexcept -> void;

    auto registerSparseFileDescriptors(unsigned int fileDescriptorCount) noexcept -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length) noexcept -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<const unsigned int> fileDescriptors) noexcept -> void;

    [[nodiscard]] auto setupBufferRing(unsigned short entries, unsigned short id) noexcept -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) noexcept -> void;

    auto submitWait(unsigned int count) noexcept -> void;

    [[nodiscard]] auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task) noexcept
            -> unsigned int;

    [[nodiscard]] auto getSqe() noexcept -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                           unsigned short bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring userRing;
};
