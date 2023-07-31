#include "BufferRing.h"

#include "../exception/Exception.h"
#include "../log/Log.h"

using std::shared_ptr;
using std::source_location;
using std::string;
using std::vector;

BufferRing::BufferRing(unsigned short entries, unsigned long bufferSize, unsigned short id,
                       const shared_ptr<UserRing> &userRing)
    : bufferRing{userRing->setupBufferRing(entries, id)},
      buffers{vector<vector<char>>(entries, vector<char>(bufferSize, 0))}, id{id},
      mask{static_cast<unsigned short>(io_uring_buf_ring_mask(static_cast<unsigned int>(entries)))}, offset{0},
      userRing{userRing} {
    for (unsigned short i{0}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->add(i);

    this->advanceCompletionBufferRingBuffer(0);
}

auto BufferRing::getId() const noexcept -> unsigned short { return this->id; }

auto BufferRing::getData(unsigned short bufferIndex, unsigned long dataSize) -> string {
    string data{this->buffers[bufferIndex].begin(), this->buffers[bufferIndex].begin() + static_cast<long>(dataSize)};

    if (dataSize == this->buffers[bufferIndex].size()) this->buffers[bufferIndex].resize(this->buffers.size() * 2);

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->bufferRing, static_cast<int>(completionCount),
                                                      this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->bufferRing != nullptr) {
        try {
            this->userRing->freeBufferRing(this->bufferRing, this->buffers.size(), this->id);
        } catch (Exception &exception) { Log::produce(exception.getMessage()); }
    }
}

auto BufferRing::add(unsigned short index) noexcept -> void {
    io_uring_buf_ring_add(this->bufferRing, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);
}
