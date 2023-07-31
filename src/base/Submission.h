#pragma once

#include <liburing.h>

class Submission {
public:
    explicit Submission(io_uring_sqe *sqe) noexcept;

    Submission(const Submission &) = delete;

    auto setUserData(unsigned long long userData) noexcept -> void;

    auto setFlags(unsigned int flags) noexcept -> void;

    auto setBufferGroup(unsigned short bufferGroup) noexcept -> void;

    auto accept(int fileDescriptor, sockaddr *address, socklen_t *addressLength, int flags) noexcept -> void;

    auto read(int fileDescriptor, void *buffer, unsigned int bufferLength, unsigned long long offset) noexcept -> void;

    auto receive(int fileDescriptor, void *buffer, unsigned long bufferLength, int flags) noexcept -> void;

    auto send(int fileDescriptor, const void *buffer, unsigned long bufferLength, int flags,
              unsigned int zeroCopyFlags) noexcept -> void;

    auto cancel(int fileDescriptor, int flags) noexcept -> void;

    auto close(int fileDescriptorIndex) noexcept -> void;

private:
    io_uring_sqe *submission;
};
