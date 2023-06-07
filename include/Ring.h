#pragma once

#include <liburing.h>

#include <vector>

class Ring {
public:
    Ring();

    Ring(const Ring &ring) = delete;

    Ring(Ring &&ring) noexcept;

    auto operator=(Ring &&ring) noexcept -> Ring &;

private:
    auto registerFileDescriptor() -> void;

    auto registerCpu() -> void;

    auto registerFileDescriptors() -> void;

public:
    auto registerBuffer(io_uring_buf_reg &reg) -> void;

    auto unregisterBuffer(int bufferId) -> void;

    auto getCompletion() -> io_uring_cqe *;

    auto consumeCompletion(io_uring_cqe *completion) -> void;

    auto getSubmission() -> io_uring_sqe *;

    auto submit() -> void;

    ~Ring();

private:
    static std::mutex lock;
    static std::vector<int> cpus;

    static thread_local bool instance;

    io_uring self;
};
