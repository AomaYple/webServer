#include "BufferRing.h"

#include "../exception/Exception.h"
#include "../log/Log.h"

using namespace std;

BufferRing::BufferRing(unsigned int entries, std::size_t bufferSize, __u16 id, const shared_ptr<UserRing> &userRing)
    : bufferRing{userRing->setupBufferRing(entries, id, 0)},
      buffers{vector<vector<byte>>(entries, vector<byte>(bufferSize, byte{0}))}, id{id},
      mask{io_uring_buf_ring_mask(entries)}, offset{0}, userRing{userRing} {
    for (std::size_t i{0}; i < this->buffers.size(); ++i) this->add(i);

    this->advanceCompletionBufferRingBuffer(0);
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : bufferRing{other.bufferRing}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask},
      offset{other.offset}, userRing{std::move(other.userRing)} {
    other.bufferRing = nullptr;
}

auto BufferRing::add(__u32 index) noexcept -> void {
    const span<byte> buffer{this->buffers[index]};

    io_uring_buf_ring_add(this->bufferRing, buffer.data(), buffer.size_bytes(), index, this->mask, this->offset++);
}

auto BufferRing::getId() const noexcept -> __u16 { return this->id; }

auto BufferRing::getData(__u32 bufferIndex, __s32 dataSize) -> vector<byte> {
    vector<byte> &buffer{this->buffers[bufferIndex]};

    vector<byte> data{buffer.begin(), buffer.begin() + dataSize};

    if (dataSize == buffer.size()) buffer.resize(buffer.size() * 2);

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(int completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->bufferRing, completionCount, this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->bufferRing != nullptr) {
        try {
            this->userRing->freeBufferRing(this->bufferRing, this->buffers.size(), this->id);
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}
