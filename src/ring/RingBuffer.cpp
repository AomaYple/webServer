#include "RingBuffer.hpp"

#include "Ring.hpp"

#include <utility>

RingBuffer::RingBuffer(unsigned int entries, unsigned int size, int id, std::shared_ptr<Ring> ring) :
    handle{ring->setupRingBuffer(entries, id)}, buffers{entries, std::vector<std::byte>{size}}, id{id},
    mask{io_uring_buf_ring_mask(entries)}, ring{std::move(ring)} {
    for (unsigned short i{}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->add(i);

    this->ring->advance(this->getHandle(), 0, std::exchange(this->offset, 0));
}

RingBuffer::RingBuffer(RingBuffer &&other) noexcept :
    handle{std::exchange(other.handle, nullptr)}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
    offset{other.offset}, ring{std::move(other.ring)} {}

auto RingBuffer::operator=(RingBuffer &&other) noexcept -> RingBuffer & {
    if (this == &other) return *this;

    this->destroy();

    this->handle = std::exchange(other.handle, nullptr);
    this->buffers = std::move(other.buffers);
    this->id = other.id;
    this->mask = other.mask;
    this->offset = other.offset;
    this->ring = std::move(other.ring);

    return *this;
}

RingBuffer::~RingBuffer() { this->destroy(); }

auto RingBuffer::getHandle() const noexcept -> io_uring_buf_ring * { return this->handle; }

auto RingBuffer::getId() const noexcept -> int { return this->id; }

auto RingBuffer::readFromBuffer(unsigned short index, unsigned int size) -> std::span<const std::byte> {
    std::vector<std::byte> &buffer{this->buffers[index]};

    buffer.resize(size * 2);
    this->add(index);

    return {buffer.cbegin(), buffer.cbegin() + size};
}

auto RingBuffer::getAdvanceBufferCount() noexcept -> int { return std::exchange(this->offset, 0); }

auto RingBuffer::destroy() const -> void {
    if (this->handle != nullptr) this->ring->freeRingBuffer(this->handle, this->buffers.size(), this->id);
}

auto RingBuffer::add(unsigned short index) noexcept -> void {
    io_uring_buf_ring_add(this->handle, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);
}
