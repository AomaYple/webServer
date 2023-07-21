#pragma once

#include <liburing.h>

#include <functional>
#include <span>

auto getFileDescriptorLimit() -> unsigned int;

class UserRing {
public:
    UserRing(unsigned int entries, io_uring_params &params);

    UserRing(const UserRing &) = delete;

    UserRing(UserRing &&) noexcept;

    auto operator=(UserRing &&) noexcept -> UserRing &;

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor() -> void;

    auto registerCpu(unsigned short cpuCode) -> void;

    auto registerFileDescriptors(unsigned int fileDescriptorCount) -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length) -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<int> fileDescriptors) -> void;

    [[nodiscard]] auto setupBufferRing(unsigned short entries, unsigned short id) -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) -> void;

    auto submitWait(unsigned int count) -> void;

    [[nodiscard]] auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task) -> unsigned int;

    [[nodiscard]] auto getSqe() -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                           unsigned short bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring userRing;
};
