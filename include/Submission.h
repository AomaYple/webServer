#pragma once

#include <memory>

#include "Ring.h"

class Submission {
public:
    explicit Submission(std::shared_ptr<Ring> &ring);

    Submission(const Submission &submission) = delete;

    Submission(Submission &&submission) noexcept;

    auto operator=(Submission &&submission) noexcept -> Submission &;

    auto setData(unsigned long long data) -> void;

    auto setFlags(unsigned int flags) -> void;

    auto setBufferId(unsigned short id) -> void;

private:
    auto judgeUsed() -> void;

public:
    auto time(__kernel_timespec *time, unsigned int count, unsigned int flags) -> void;

    auto updateTime(__kernel_timespec *time, unsigned long long data, unsigned int flags) -> void;

    auto accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) -> void;

    auto receive(int socket, void *buffer, unsigned long length, int flags) -> void;

    auto send(int socket, const void *buffer, unsigned long length, int flags, int zeroCopyFlags) -> void;

    auto cancel(int fileDescriptor, int flags) -> void;

    auto close(int fileIndex) -> void;

    ~Submission();

private:
    io_uring_sqe *self;
    bool used;
    std::shared_ptr<Ring> ring;
};
