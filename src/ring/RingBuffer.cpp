#include "RingBuffer.hpp"

#include <utility>

RingBuffer::RingBuffer(unsigned int entries, unsigned int bufferSize, int id, std::shared_ptr<Ring> ring)
    : handle{ring->setupRingBuffer(entries, id)},
      buffers{std::vector<std::vector<std::byte>>(entries, std::vector<std::byte>(bufferSize, std::byte{0}))}, id{id},
      mask{io_uring_buf_ring_mask(entries)}, offset{0}, ring{std::move(ring)} {
    for (unsigned short i{0}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->addBuffer(i);
    this->advanceBuffer();
}

RingBuffer::RingBuffer(RingBuffer &&other) noexcept
    : handle{std::exchange(other.handle, nullptr)}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
      offset{other.offset}, ring{std::move(other.ring)} {}

auto RingBuffer::operator=(RingBuffer &&other) noexcept -> RingBuffer & {
    if (this != &other) {
        this->destroy();

        this->handle = std::exchange(other.handle, nullptr);
        this->buffers = std::move(other.buffers);
        this->id = other.id;
        this->mask = other.mask;
        this->offset = other.offset;
        this->ring = std::move(other.ring);
    }

    return *this;
}

RingBuffer::~RingBuffer() { this->destroy(); }

auto RingBuffer::getId() const noexcept -> int { return this->id; }

auto RingBuffer::getData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte> {
    std::vector<std::byte> &buffer{this->buffers[index]};
    std::vector<std::byte> data{buffer.cbegin(), buffer.cbegin() + dataSize};

    buffer.resize(dataSize * 2);

    this->addBuffer(index);
    this->advanceBuffer();

    return data;
}

auto RingBuffer::destroy() const -> void {
    if (this->handle != nullptr) this->ring->freeRingBuffer(this->handle, this->buffers.size(), this->id);
}

auto RingBuffer::addBuffer(unsigned short index) noexcept -> void {
    io_uring_buf_ring_add(this->handle, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);
}

auto RingBuffer::advanceBuffer() noexcept -> void {
    io_uring_buf_ring_advance(this->handle, std::exchange(this->offset, 0));
}
