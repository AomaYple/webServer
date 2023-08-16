#pragma once

#include "UserRing.h"

#include <memory>

class BufferRing {
public:
    BufferRing(unsigned int entries, std::size_t bufferSize, __u16 id, const std::shared_ptr<UserRing> &userRing);

    BufferRing(const BufferRing &) = delete;

    BufferRing(BufferRing &&) noexcept;

private:
    auto add(__u32 index) noexcept -> void;

public:
    [[nodiscard]] auto getId() const noexcept -> __u16;

    [[nodiscard]] auto getData(__u32 bufferIndex, __s32 dataSize) -> std::vector<std::byte>;

    auto advanceCompletionBufferRingBuffer(int completionCount) noexcept -> void;

    ~BufferRing();

private:
    io_uring_buf_ring *bufferRing;
    std::vector<std::vector<std::byte>> buffers;
    const __u16 id;
    const int mask;
    int offset;
    std::shared_ptr<UserRing> userRing;
};
