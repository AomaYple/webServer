#pragma once

#include <liburing.h>

class Submission {
public:
    explicit Submission(io_uring_sqe *submission);

    Submission(const Submission &other) = delete;

    Submission(Submission &&other) noexcept;

    auto operator=(Submission &&other) noexcept -> Submission &;

    auto setData(unsigned long long data) -> void;

    auto setFlags(unsigned int flags) -> void;

    auto accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) -> void;

private:
    io_uring_sqe *self;
};
