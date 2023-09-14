#pragma once

#include <liburing.h>

#include <span>

class Submission {
public:
    Submission(io_uring_sqe *sqe, int fileDescriptor, sockaddr *address, socklen_t *addressLength, int flags) noexcept;

    Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<std::byte> buffer, unsigned long offset) noexcept;

    Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<std::byte> buffer, int flags) noexcept;

    Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<const std::byte> buffer, int flags,
               unsigned int zeroCopyFlags) noexcept;

    Submission(io_uring_sqe *sqe, int fileDescriptor, unsigned int flags) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptorIndex) noexcept;

    Submission(const Submission &) = delete;

    Submission(Submission &&) noexcept = default;

    auto setUserData(unsigned long userData) const noexcept -> void;

    auto setFlags(unsigned int flags) const noexcept -> void;

    auto setBufferRingId(unsigned short bufferRingId) const noexcept -> void;

private:
    io_uring_sqe *const submission;
};
