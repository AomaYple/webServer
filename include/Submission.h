#pragma once

#include <liburing.h>

class Ring;

class Submission {
public:
    explicit Submission(Ring &ring);

    Submission(const Submission &other) = delete;

    Submission(Submission &&other) noexcept;

    auto operator=(Submission &&other) noexcept -> Submission &;

    auto setData(unsigned long long data) -> void;

    auto setFlags(unsigned int flags) -> void;

    auto setBufferGroup(unsigned short id) -> void;

    auto accept(int fileDescriptor, sockaddr *address, socklen_t *addressLength, int flags) -> void;

    auto receive(int fileDescriptor, void *buffer, unsigned long length, int flags) -> void;

    auto close(int fileDescriptor) -> void;

    auto timeout(__kernel_timespec *timespec, unsigned int count, unsigned int flags) -> void;

    auto updateTimeout(__kernel_timespec *timespec, unsigned long long data, unsigned int flags) -> void;

    auto removeTimeout(unsigned long long data, unsigned int flags) -> void;

    auto cancelFileDescriptor(int fileDescriptor, int flags) -> void;

private:
    auto judgeUsed() -> void;

    io_uring_sqe *self;
    bool used;
};
