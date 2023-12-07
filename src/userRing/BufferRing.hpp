#pragma once

#include "UserRing.hpp"

#include <memory>

class BufferRing {
public:
    BufferRing(unsigned int entries, unsigned int bufferSize, int id,
               const std::shared_ptr<UserRing> &userRing) noexcept;

    BufferRing(const BufferRing &) = delete;

    auto operator=(const BufferRing &) -> BufferRing & = delete;

    BufferRing(BufferRing &&other) noexcept;

    auto operator=(BufferRing &&other) noexcept -> BufferRing &;

    ~BufferRing();

    [[nodiscard]] auto getId() const noexcept -> int;

    [[nodiscard]] auto getData(unsigned short bufferIndex, unsigned int dataSize) noexcept -> std::vector<std::byte>;

private:
    auto destroy() const noexcept -> void;

    auto addBuffer(unsigned short index) noexcept -> void;

    auto advanceBuffer() noexcept -> void;

    io_uring_buf_ring *handle;
    std::vector<std::vector<std::byte>> buffers;
    int id, mask, offset;
    std::shared_ptr<UserRing> userRing;
};
