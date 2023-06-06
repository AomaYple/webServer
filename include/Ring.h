#pragma once

#include <liburing.h>

#include <functional>
#include <vector>

class Completion;

class Ring {
public:
    Ring();

    Ring(const Ring &ring) = delete;

    Ring(Ring &&ring) noexcept;

    auto operator=(Ring &&ring) noexcept -> Ring &;

    auto registerBuffer(io_uring_buf_reg &reg) -> void;

    auto unregisterBuffer(int bufferId) -> void;

    auto forEach(const std::function<auto(const Completion &)->bool> &task) -> std::pair<int, unsigned int>;

    auto getSubmission() -> io_uring_sqe *;

    auto advanceBufferCompletion(io_uring_buf_ring *buffer, int number) -> void;

    auto advanceCompletion(unsigned int number) -> void;

    ~Ring();

private:
    auto registerFileDescriptor() -> void;

    auto registerCpu() -> void;

    auto registerFileDescriptors() -> void;

    auto submitWait() -> void;

    static std::mutex lock;
    static std::vector<int> cpus;

    static thread_local bool instance;

    io_uring self;
};
