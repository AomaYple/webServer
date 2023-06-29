#pragma once

#include <liburing.h>

#include <functional>
#include <span>

auto getFileDescriptorLimit() -> unsigned int;

class UserRing {
public:
    UserRing(unsigned int entries, io_uring_params &params);

    UserRing(const UserRing &other) = delete;

    UserRing(UserRing &&other) noexcept;

    auto operator=(UserRing &&other) noexcept -> UserRing &;

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor() -> void;

    auto registerCpu(unsigned short cpuCode) -> void;

    auto registerFileDescriptors(unsigned int fileDescriptorCount) -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length) -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<int> fileDescriptors) -> void;

    auto setupBufferRing(unsigned short entries, unsigned short id) -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) -> void;

    auto submitWait(unsigned int waitCount) -> void;

    auto forEachCompletion(const std::function<auto(io_uring_cqe *cqe)->void> &task) noexcept -> unsigned int;

    auto getSubmission() -> io_uring_sqe *;

    auto advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                           unsigned short bufferRingBufferCount) noexcept -> void;

    ~UserRing();

private:
    io_uring self;
};
