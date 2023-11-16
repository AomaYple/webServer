#pragma once

#include "UserRing.hpp"

#include <memory>

class BufferRing {
public:
    BufferRing(unsigned short bufferCount, unsigned int bufferSize, unsigned short id,
               const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    BufferRing(BufferRing &&) = default;

    auto operator=(const BufferRing &) -> BufferRing & = delete;

    auto operator=(BufferRing &&) noexcept -> BufferRing &;

    ~BufferRing();

    [[nodiscard]] auto getId() const noexcept -> unsigned short;

    [[nodiscard]] auto getData(unsigned short bufferIndex, unsigned int dataSize) -> std::vector<std::byte>;

    auto advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void;

private:
    auto destroy() const -> void;

    auto add(unsigned short index) noexcept -> void;

    io_uring_buf_ring *bufferRing;
    std::vector<std::vector<std::byte>> buffers;
    unsigned short id, mask, offset;
    std::shared_ptr<UserRing> userRing;
};
