#pragma once

#include <liburing.h>

#include <functional>
#include <source_location>
#include <span>

class Completion;
class Submission;

class Ring {
public:
    [[nodiscard]] static auto
    getFileDescriptorLimit(std::source_location sourceLocation = std::source_location::current()) -> unsigned long;

    Ring(unsigned int entries, io_uring_params &params);

    Ring(const Ring &) = delete;

    auto operator=(const Ring &) -> Ring & = delete;

    Ring(Ring &&) noexcept;

    auto operator=(Ring &&) noexcept -> Ring &;

    ~Ring();

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    auto registerSelfFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto registerCpu(unsigned short cpuCode, std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto registerSparseFileDescriptor(unsigned int count,
                                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto allocateFileDescriptorRange(unsigned int offset, unsigned int length,
                                     std::source_location sourceLocation = std::source_location::current()) -> void;

    auto updateFileDescriptors(unsigned int offset, std::span<const int> fileDescriptors,
                               std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto setupRingBuffer(unsigned int entries, int id,
                                       std::source_location sourceLocation = std::source_location::current())
            -> io_uring_buf_ring *;

    auto freeRingBuffer(io_uring_buf_ring *ringBufferHandle, unsigned int entries, int id,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

    auto submit(const Submission &submission) -> void;

    auto traverseCompletion(const std::function<auto(const Completion &completion)->void> &task) -> void;

private:
    [[nodiscard]] static auto initialize(unsigned int entries, io_uring_params &params,
                                         std::source_location sourceLocation = std::source_location::current())
            -> io_uring;

    auto destroy() noexcept -> void;

    [[nodiscard]] auto getSqe(std::source_location sourceLocation = std::source_location::current()) -> io_uring_sqe *;

    auto wait(unsigned int count, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto advanceCompletion(int count) noexcept -> void;

    io_uring handle;
};
