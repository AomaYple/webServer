#include "BufferRing.h"

#include "Log.h"

using std::runtime_error;
using std::shared_ptr;
using std::source_location;
using std::string, std::vector;

BufferRing::BufferRing(unsigned short entries, unsigned long bufferSize, unsigned short id,
                       const shared_ptr<UserRing> &userRing)
    : self{userRing->setupBufferRing(entries, id)}, buffers{vector<vector<char>>(entries, vector<char>(bufferSize, 0))},
      id{id}, mask{static_cast<unsigned short>(io_uring_buf_ring_mask(static_cast<unsigned int>(entries)))}, offset{0},
      userRing{userRing} {
    for (unsigned short i{0}; i < static_cast<unsigned short>(this->buffers.size()); ++i) this->add(i);

    io_uring_buf_ring_advance(this->self, this->offset);

    this->offset = 0;
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : self{other.self}, buffers{std::move(other.buffers)}, id{other.id}, mask{other.mask}, offset{other.offset},
      userRing{std::move(other.userRing)} {
    other.self = nullptr;
}

auto BufferRing::operator=(BufferRing &&other) noexcept -> BufferRing & {
    if (this != &other) {
        this->self = other.self;
        this->buffers = std::move(other.buffers);
        this->id = other.id;
        this->mask = other.mask;
        this->offset = other.offset;
        this->userRing = std::move(other.userRing);
        other.self = nullptr;
    }
    return *this;
}

auto BufferRing::getId() const noexcept -> unsigned short { return this->id; }

auto BufferRing::getData(unsigned short bufferIndex, unsigned long dataSize) -> string {
    string data{this->buffers[bufferIndex].begin(), this->buffers[bufferIndex].begin() + static_cast<long>(dataSize)};

    if (dataSize == this->buffers[bufferIndex].size()) this->buffers[bufferIndex].resize(this->buffers.size() * 2);

    this->add(bufferIndex);

    return data;
}

auto BufferRing::advanceCompletionBufferRingBuffer(unsigned int completionCount) noexcept -> void {
    this->userRing->advanceCompletionBufferRingBuffer(this->self, static_cast<int>(completionCount), this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->self != nullptr) {
        try {
            this->userRing->freeBufferRing(this->self, this->buffers.size(), this->id);
        } catch (const runtime_error &runtimeError) {
            Log::produce(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }
}

auto BufferRing::add(unsigned short index) noexcept -> void {
    io_uring_buf_ring_add(this->self, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);
}
