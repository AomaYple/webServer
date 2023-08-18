#pragma once

#include <liburing.h>

#include <span>

class Submission {
public:
    Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, sockaddr *address, socklen_t *addressLength,
               unsigned int flags) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, std::span<std::byte> buffer,
               unsigned long offset) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, std::span<std::byte> buffer,
               unsigned int flags) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, std::span<const std::byte> buffer, unsigned int flags,
               unsigned char zeroCopyFlags) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, unsigned char flags) noexcept;

    Submission(io_uring_sqe *sqe, unsigned int fileDescriptorIndex) noexcept;

    Submission(const Submission &) = delete;

    Submission(Submission &&) noexcept = default;

    auto setUserData(unsigned long userData) const noexcept -> void;

    auto setFlags(unsigned char flags) const noexcept -> void;

    auto setBufferRingId(unsigned short bufferRingId) const noexcept -> void;

private:
    io_uring_sqe *const submission;
};
