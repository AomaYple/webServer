#pragma once

#include "Ring.hpp"

#include <memory>

class RingBuffer {
public:
    RingBuffer(unsigned int entries, unsigned int size, int id, std::shared_ptr<Ring> ring);

    RingBuffer(const RingBuffer &) = delete;

    auto operator=(const RingBuffer &) -> RingBuffer & = delete;

    RingBuffer(RingBuffer &&other) noexcept;

    auto operator=(RingBuffer &&other) noexcept -> RingBuffer &;

    ~RingBuffer();

    [[nodiscard]] auto getId() const noexcept -> int;

    [[nodiscard]] auto readFromBuffer(unsigned short index, unsigned int size) -> std::vector<std::byte>;

private:
    auto destroy() const -> void;

    auto add(unsigned short index) -> void;

    auto advance() -> void;

    io_uring_buf_ring *handle;
    std::vector<std::vector<std::byte>> buffers;
    int id, mask, offset{0};
    std::shared_ptr<Ring> ring;
};
