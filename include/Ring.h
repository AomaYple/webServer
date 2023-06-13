#pragma once

#include <liburing.h>

#include <functional>
#include <source_location>

class Completion;

class Ring {
public:
    Ring();

    Ring(const Ring &other) = delete;

    Ring(Ring &&other) noexcept;

    auto operator=(Ring &&other) noexcept -> Ring &;

    auto setupBufferRing(unsigned int number, int id) -> io_uring_buf_ring *;

    auto freeBufferRing(io_uring_buf_ring *bufferRing, unsigned int number, int id) -> void;

    auto updateFileDescriptor(int fileDescriptor) -> void;

    auto forEach(
        const std::function<auto(const Completion &completion, std::source_location sourceLocation)->void> &task)
        -> int;

    auto getSubmission() -> io_uring_sqe *;

    auto advanceCompletionBufferRing(io_uring_buf_ring *bufferRing, int completionNumber, int bufferRingNumber) -> void;

    ~Ring();

private:
    static std::mutex lock;
    static std::vector<int> values;

    static thread_local bool instance;

    auto registerCpu() -> void;

    auto registerMaxWorks() -> void;

    auto registerRingFileDescriptor() -> void;

    auto registerSparseFileDescriptors() -> void;

    auto submitWait() -> void;

    io_uring self;
};
