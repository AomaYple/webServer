#pragma once

#include "Ring.hpp"

#include <memory>

class RingBuffer {
public:
    RingBuffer(unsigned int entries, unsigned int bufferSize, int id, std::shared_ptr<Ring> ring);

    RingBuffer(const RingBuffer &) = delete;

    auto operator=(const RingBuffer &) -> RingBuffer & = delete;

    RingBuffer(RingBuffer &&other) noexcept;

    auto operator=(RingBuffer &&other) noexcept -> RingBuffer &;

    ~RingBuffer();

    [[nodiscard]] auto getId() const noexcept -> int;

    [[nodiscard]] auto getData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte>;

private:
    auto destroy() const -> void;

    auto addBuffer(unsigned short index) noexcept -> void;

    auto advanceBuffer() noexcept -> void;

    io_uring_buf_ring *handle;
    std::vector<std::vector<std::byte>> buffers;
    int id, mask, offset;
    std::shared_ptr<Ring> ring;
};
