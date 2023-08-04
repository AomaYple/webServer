#include "BufferRing.h"

#include "../exception/Exception.h"
#include "../log/Log.h"

using std::shared_ptr;
using std::string;
using std::vector;

BufferRing::BufferRing(std::uint_fast16_t entries, std::uint_fast64_t bufferSize, std::uint_fast16_t id,
                       const shared_ptr<UserRing> &userRing)
    : bufferRing{userRing->setupBufferRing(entries, id)},
      buffers{vector<vector<std::int_fast8_t>>(entries, vector<std::int_fast8_t>(bufferSize, 0))}, id{id},
      mask{static_cast<std::uint_fast16_t>(io_uring_buf_ring_mask(entries))}, offset{0}, userRing{userRing} {
    for (std::uint_fast16_t i{0}; i < this->buffers.size(); ++i) this->add(i);

    this->advanceCompletionBufferRingBuffer(0);
}

auto BufferRing::getId() const noexcept -> std::uint_fast16_t { return this->id; }

auto BufferRing::getData(std::uint_fast16_t bufferIndex, std::uint_fast64_t dataSize) -> string {
    string data{this->buffers[bufferIndex].begin(),
                this->buffers[bufferIndex].begin() + static_cast<std::int_fast64_t>(dataSize)};

    if (dataSize == this->buffers[bufferIndex].size()) this->buffers[bufferIndex].resize(this->buffers.size() * 2);

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(std::uint_fast32_t completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->bufferRing, completionCount, this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->bufferRing != nullptr) {
        try {
            this->userRing->freeBufferRing(this->bufferRing, this->buffers.size(), this->id);
        } catch (Exception &exception) { Log::produce(exception.getMessage()); }
    }
}

auto BufferRing::add(std::uint_fast16_t index) noexcept -> void {
    io_uring_buf_ring_add(this->bufferRing, this->buffers[index].data(), this->buffers[index].size(), index,
                          static_cast<int>(this->mask), static_cast<int>(this->offset++));
}
