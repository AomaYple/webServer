#pragma once

#include <memory>

#include "UserRing.h"

class BufferRing {
public:
    BufferRing(unsigned short entries, unsigned long bufferSize, unsigned short id,
               const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    [[nodiscard]] auto getId() const noexcept -> unsigned short;

    [[nodiscard]] auto getData(unsigned short bufferIndex, unsigned long dataSize) -> std::string;

    auto advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void;

    ~BufferRing();

private:
    auto add(unsigned short index) noexcept -> void;

    io_uring_buf_ring *bufferRing;
    std::vector<std::vector<char>> buffers;
    unsigned short id, mask, offset;
    std::shared_ptr<UserRing> userRing;
};
