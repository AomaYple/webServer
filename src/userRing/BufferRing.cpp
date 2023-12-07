#include "BufferRing.hpp"

#include <utility>

BufferRing::BufferRing(unsigned int entries, unsigned int bufferSize, int id,
                       const std::shared_ptr<UserRing> &userRing) noexcept
    : handle{userRing->setupBufferRing(entries, id)},
      buffers{std::vector<std::vector<std::byte>>(entries, std::vector<std::byte>(bufferSize, std::byte{0}))}, id{id},
      mask{io_uring_buf_ring_mask(entries)}, offset{0}, userRing{userRing} {
    for (unsigned short i{0}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->addBuffer(i);
    this->advanceBuffer();
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : handle{std::exchange(other.handle, nullptr)}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
      offset{other.offset}, userRing{std::move(other.userRing)} {}

auto BufferRing::operator=(BufferRing &&other) noexcept -> BufferRing & {
    if (this != &other) {
        this->destroy();

        this->handle = std::exchange(other.handle, nullptr);
        this->buffers = std::move(other.buffers);
        this->id = other.id;
        this->mask = other.mask;
        this->offset = other.offset;
        this->userRing = std::move(other.userRing);
    }

    return *this;
}

BufferRing::~BufferRing() { this->destroy(); }

auto BufferRing::getId() const noexcept -> int { return this->id; }

auto BufferRing::getData(unsigned short bufferIndex, unsigned int dataSize) noexcept -> std::vector<std::byte> {
    std::vector<std::byte> &buffer{this->buffers[bufferIndex]};

    std::vector<std::byte> data{buffer.cbegin(), buffer.cbegin() + dataSize};

    buffer.resize(dataSize * 2);

    this->addBuffer(bufferIndex);
    this->advanceBuffer();

    return data;
}

auto BufferRing::destroy() const noexcept -> void {
    if (this->userRing) this->userRing->freeBufferRing(this->handle, this->buffers.size(), this->id);
}

auto BufferRing::addBuffer(unsigned short index) noexcept -> void {
    io_uring_buf_ring_add(this->handle, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);
}

auto BufferRing::advanceBuffer() noexcept -> void {
    io_uring_buf_ring_advance(this->handle, std::exchange(this->offset, 0));
}
