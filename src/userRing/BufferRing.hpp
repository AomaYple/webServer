#pragma once

#include "UserRing.hpp"

#include <memory>

class BufferRing {
public:
    BufferRing(unsigned short bufferCount, unsigned int bufferSize, unsigned short id,
               const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    BufferRing(BufferRing &&) noexcept;

private:
    auto add(unsigned short index) noexcept -> void;

public:
    [[nodiscard]] auto getId() const noexcept -> unsigned short;

    [[nodiscard]] auto getData(unsigned short bufferIndex, unsigned int dataSize) -> std::vector<std::byte>;

    auto advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void;

    ~BufferRing();

private:
    io_uring_buf_ring *const bufferRing;
    std::vector<std::vector<std::byte>> buffers;
    const unsigned short id, mask;
    unsigned short offset;
    std::shared_ptr<UserRing> userRing;
};