#pragma once

#include <memory>

#include "Ring.h"

class BufferRing {
public:
    explicit BufferRing(const std::shared_ptr<Ring> &ring);

    BufferRing(const BufferRing &other) = delete;

    BufferRing(BufferRing &&other) noexcept;

    auto operator=(BufferRing &&other) noexcept -> BufferRing &;

    [[nodiscard]] auto getId() const -> int;

    auto getData(unsigned short index, int size) -> std::string;

    auto advanceCompletionBufferRing(int completionNumber) -> void;

    ~BufferRing();

private:
    io_uring_buf_ring *self;
    std::vector<std::vector<char>> bufferRings;
    int id, mask, offset;
    std::shared_ptr<Ring> ring;
};
