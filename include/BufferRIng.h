#pragma once

#include <memory>

#include "Ring.h"

class BufferRing {
public:
    BufferRing(unsigned int bufferNumber, unsigned int bufferSize, int id, const std::shared_ptr<Ring> &ring);

    BufferRing(const BufferRing &other) = delete;

    BufferRing(BufferRing &&other) noexcept;

    auto operator=(BufferRing &&other) noexcept -> BufferRing &;

    [[nodiscard]] auto getId() const -> int;

    auto getData(unsigned short index, unsigned int size) -> std::string;

    auto advance(int completionNumber) -> void;

    ~BufferRing();

private:
    auto allocBuffers() -> void;

    io_uring_buf_ring *self;
    char **buffers;
    unsigned int bufferNumber, bufferSize;
    int id, mask, offset;
    std::shared_ptr<Ring> ring;
};
