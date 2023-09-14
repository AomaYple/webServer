#include "BufferRing.hpp"

BufferRing::BufferRing(unsigned short bufferCount, unsigned int bufferSize, unsigned short id,
                       const std::shared_ptr<UserRing> &userRing)
    : bufferRing{userRing->setupBufferRing(bufferCount, id)},
      buffers{std::vector<std::vector<std::byte>>(bufferCount, std::vector<std::byte>(bufferSize, std::byte{0}))},
      id{id}, mask{static_cast<unsigned short>(io_uring_buf_ring_mask(bufferCount))}, offset{0}, userRing{userRing} {
    for (unsigned short i{0}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->add(i);

    this->advanceCompletionBufferRingBuffer(0);
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : bufferRing{other.bufferRing}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
      offset{other.offset}, userRing{std::move(other.userRing)} {}

auto BufferRing::add(unsigned short index) noexcept -> void {
    const std::span<std::byte> buffer{this->buffers[index]};

    io_uring_buf_ring_add(this->bufferRing, buffer.data(), buffer.size_bytes(), index, this->mask, this->offset++);
}

auto BufferRing::getId() const noexcept -> unsigned short { return this->id; }

auto BufferRing::getData(unsigned short bufferIndex, unsigned int dataSize) -> std::vector<std::byte> {
    const std::span<const std::byte> buffer{this->buffers[bufferIndex]};

    std::vector<std::byte> data{buffer.begin(), buffer.begin() + dataSize};

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->bufferRing, static_cast<int>(completionCount),
                                                      this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->userRing != nullptr) this->userRing->freeBufferRing(this->bufferRing, this->buffers.size(), this->id);
}
