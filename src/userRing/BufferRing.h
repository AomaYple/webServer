#pragma once

#include "UserRing.h"

#include <memory>

class BufferRing {
public:
    BufferRing(uint16_t bufferCount, uint32_t bufferSize, uint16_t id, const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    BufferRing(BufferRing &&) noexcept;

private:
    auto add(uint16_t index) noexcept -> void;

public:
    [[nodiscard]] auto getId() const noexcept -> uint16_t;

    [[nodiscard]] auto getData(uint16_t bufferIndex, uint32_t dataSize) -> std::vector<std::byte>;

    auto advanceCompletionBufferRingBuffer(uint32_t completionCount) noexcept -> void;

    ~BufferRing();

private:
    io_uring_buf_ring *const bufferRing;
    std::vector<std::vector<std::byte>> buffers;
    const uint16_t id, mask;
    uint16_t offset;
    std::shared_ptr<UserRing> userRing;
};
