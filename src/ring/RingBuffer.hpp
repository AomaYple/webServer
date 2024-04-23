#pragma once

#include <liburing/io_uring.h>
#include <memory>
#include <span>
#include <vector>

class Ring;

class RingBuffer {
public:
    RingBuffer(unsigned int entries, unsigned int size, int id, std::shared_ptr<Ring> ring);

    RingBuffer(const RingBuffer &) = delete;

    RingBuffer(RingBuffer &&other) noexcept;

    auto operator=(const RingBuffer &) = delete;

    auto operator=(RingBuffer &&other) noexcept -> RingBuffer &;

    ~RingBuffer();

    [[nodiscard]] auto getHandle() const noexcept -> io_uring_buf_ring *;

    [[nodiscard]] auto getId() const noexcept -> int;

    [[nodiscard]] auto readFromBuffer(unsigned short index, unsigned int size) -> std::span<const std::byte>;

    auto getAdvanceBufferCount() noexcept -> int;

private:
    auto destroy() const -> void;

    auto add(unsigned short index) noexcept -> void;

    io_uring_buf_ring *handle;
    std::vector<std::vector<std::byte>> buffers;
    int id, mask, offset{};
    std::shared_ptr<Ring> ring;
};
