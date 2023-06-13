#include "BufferRing.h"

#include "Log.h"

using std::vector, std::string, std::shared_ptr, std::runtime_error, std::source_location;

constexpr unsigned int bufferRingNumber{1024}, bufferRingSize{1024};

constexpr int bufferRingId{0};

BufferRing::BufferRing(const shared_ptr<Ring> &ring)
    : self{ring->setupBufferRing(bufferRingNumber, bufferRingId)},
      bufferRings{bufferRingNumber, vector<char>(bufferRingSize, 0)},
      id{bufferRingId},
      mask{io_uring_buf_ring_mask(bufferRingNumber)},
      offset{0},
      ring{ring} {
    io_uring_buf_ring_init(this->self);

    for (unsigned int i{0}; i < bufferRingNumber; ++i)
        io_uring_buf_ring_add(this->self, this->bufferRings[i].data(), this->bufferRings[i].size(), i, this->mask,
                              static_cast<int>(i));

    io_uring_buf_ring_advance(this->self, static_cast<int>(this->bufferRings.size()));
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : self{other.self},
      bufferRings{std::move(other.bufferRings)},
      id{other.id},
      mask{other.mask},
      offset{other.offset},
      ring{std::move(other.ring)} {
    other.self = nullptr;
}

auto BufferRing::operator=(BufferRing &&other) noexcept -> BufferRing & {
    if (this != &other) {
        this->self = other.self;
        this->bufferRings = std::move(other.bufferRings);
        this->id = other.id;
        this->ring = std::move(other.ring);
        other.self = nullptr;
    }
    return *this;
}

auto BufferRing::getId() const -> int { return this->id; }

auto BufferRing::getData(unsigned short index, int size) -> string {
    string data{this->bufferRings[index].begin(), this->bufferRings[index].begin() + size};

    io_uring_buf_ring_add(this->self, this->bufferRings[index].data(), this->bufferRings[index].size(), index,
                          this->mask, ++this->offset);

    return data;
}

auto BufferRing::advanceCompletionBufferRing(int completionNumber) -> void {
    this->ring->advanceCompletionBufferRing(this->self, completionNumber, this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->self != nullptr) {
        try {
            ring->freeBufferRing(this->self, this->bufferRings.size(), this->id);
        } catch (runtime_error &runtimeError) {
            Log::add(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }
}
