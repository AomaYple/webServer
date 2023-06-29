#pragma once

#include <liburing.h>

class Submission {
public:
    explicit Submission(io_uring_sqe *sqe) noexcept;

    Submission(const Submission &other) = delete;

    Submission(Submission &&other) noexcept;

    auto operator=(Submission &&other) noexcept -> Submission &;

    auto setUserData(unsigned long long userData) noexcept -> void;

    auto setFlags(unsigned int flags) noexcept -> void;

    auto setBufferGroup(unsigned short bufferGroup) noexcept -> void;

    auto accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) noexcept -> void;

    auto receive(int socket, void *buffer, unsigned long bufferLength, int flags) noexcept -> void;

    auto send(int socket, const void *buffer, unsigned long bufferLength, int flags,
              unsigned int zeroCopyFlags) noexcept -> void;

    auto cancel(int socket, int flags) noexcept -> void;

    auto close(int socket) noexcept -> void;

private:
    io_uring_sqe *self;
};
