#pragma once

#include "UserRing.h"

#include <memory>

class BufferRing {
public:
    BufferRing(std::uint_fast16_t entries, std::uint_fast64_t bufferSize, std::uint_fast16_t id,
               const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    [[nodiscard]] auto getId() const noexcept -> std::uint_fast16_t;

    [[nodiscard]] auto getData(std::uint_fast16_t bufferIndex, std::uint_fast64_t dataSize) -> std::string;

    auto advanceCompletionBufferRingBuffer(std::uint_fast32_t completionCount) noexcept -> void;

    ~BufferRing();

private:
    auto add(std::uint_fast16_t index) noexcept -> void;

    io_uring_buf_ring *bufferRing;
    std::vector<std::vector<std::int_fast8_t>> buffers;
    std::uint_fast16_t id, mask, offset;
    std::shared_ptr<UserRing> userRing;
};
