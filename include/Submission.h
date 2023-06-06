#pragma once

#include <liburing.h>

class Ring;

class Submission {
public:
    explicit Submission(Ring &ring);

    Submission(const Submission &submission) = delete;

    Submission(Submission &&submission) noexcept;

    auto operator=(Submission &&submission) noexcept -> Submission &;

    auto setData(void *data) -> void;

    auto setFlags(unsigned int flags) -> void;

    auto setBufferId(unsigned short id) -> void;

    auto time(__kernel_timespec *time, unsigned int count, unsigned int flags) -> void;

    auto updateTime(__kernel_timespec *time, unsigned long long data, unsigned int flags) -> void;

    auto removeTime(unsigned long long data, unsigned int flags) -> void;

    auto accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) -> void;

    auto receive(int socket, void *buffer, unsigned long length, int flags) -> void;

    auto send(int socket, const void *buffer, unsigned long length, int flags, int zeroCopyFlags) -> void;

    auto close(int fileIndex) -> void;

    auto cancel(void *data, int flags) -> void;

private:
    auto judgeUsed() -> void;

    io_uring_sqe *self;
    bool used;
};
