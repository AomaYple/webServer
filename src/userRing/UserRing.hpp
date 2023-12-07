#pragma once

#include <liburing.h>

#include <functional>
#include <source_location>
#include <span>

class Completion;
class Submission;

class UserRing {
public:
    [[nodiscard]] static auto
    getFileDescriptorLimit(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> unsigned long;

    UserRing(unsigned int entries, io_uring_params &params) noexcept;

    UserRing(const UserRing &) = delete;

    auto operator=(const UserRing &) -> UserRing & = delete;

    UserRing(UserRing &&) noexcept;

    auto operator=(UserRing &&) noexcept -> UserRing &;

    ~UserRing();

    [[nodiscard]] auto getSelfFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> void;

    auto registerCpu(unsigned short cpuCode,
                     std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    auto registerSparseFileDescriptors(unsigned int fileDescriptorCount,
                                       std::source_location sourceLocation = std::source_location::current()) noexcept
            -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length,
                                     std::source_location sourceLocation = std::source_location::current()) noexcept
            -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<const int> fileDescriptors,
                               std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    [[nodiscard]] auto setupBufferRing(unsigned int entries, int id,
                                       std::source_location sourceLocation = std::source_location::current()) noexcept
            -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRingHandle, unsigned int entries, int id,
                        std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    auto addSubmission(const Submission &submission) noexcept -> void;

    auto submitWait(unsigned int count, std::source_location sourceLocation = std::source_location::current()) noexcept
            -> void;

    auto traverseCompletion(const std::function<auto(const Completion &completion)->void> &task) noexcept -> void;

private:
    [[nodiscard]] static auto initialize(unsigned int entries, io_uring_params &params,
                                         std::source_location sourceLocation = std::source_location::current()) noexcept
            -> io_uring;

    auto destroy() noexcept -> void;

    [[nodiscard]] auto getSqe(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> io_uring_sqe *;

    auto advanceCompletion(int count) noexcept -> void;

    io_uring handle;
};
