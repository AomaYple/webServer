#include "BufferRing.h"

using namespace std;

BufferRing::BufferRing(uint16_t bufferCount, uint32_t bufferSize, uint16_t id, const shared_ptr<UserRing> &userRing)
    : bufferRing{userRing->setupBufferRing(bufferCount, id)},
      buffers{vector<vector<byte>>(bufferCount, vector<byte>(bufferSize, byte{0}))}, id{id},
      mask{static_cast<uint16_t>(io_uring_buf_ring_mask(bufferCount))}, offset{0}, userRing{userRing} {
    for (uint16_t i{0}; i < static_cast<uint16_t>(this->buffers.size()); ++i) this->add(i);

    this->advanceCompletionBufferRingBuffer(0);
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : bufferRing{other.bufferRing}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
      offset{other.offset}, userRing{std::move(other.userRing)} {}

auto BufferRing::add(uint16_t index) noexcept -> void {
    const span<byte> buffer{this->buffers[index]};

    io_uring_buf_ring_add(this->bufferRing, buffer.data(), buffer.size_bytes(), index, this->mask, this->offset++);
}

auto BufferRing::getId() const noexcept -> uint16_t { return this->id; }

auto BufferRing::getData(uint16_t bufferIndex, uint32_t dataSize) -> vector<byte> {
    const span<const byte> buffer{this->buffers[bufferIndex]};

    vector<byte> data{buffer.begin(), buffer.begin() + dataSize};

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(uint32_t completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->bufferRing, static_cast<int>(completionCount),
                                                      this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->userRing != nullptr) this->userRing->freeBufferRing(this->bufferRing, this->buffers.size(), this->id);
}
