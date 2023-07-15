#include "BufferRIng.h"

#include <cstring>

#include "Log.h"

using std::string, std::shared_ptr, std::aligned_alloc, std::runtime_error, std::source_location;

BufferRing::BufferRing(unsigned int bufferNumber, unsigned int bufferSize, int id, const shared_ptr<Ring> &ring)
    : self{nullptr}, buffers{static_cast<char **>(aligned_alloc(sysconf(_SC_PAGESIZE), bufferNumber * sizeof(char *)))},
      bufferNumber{bufferNumber}, bufferSize{bufferSize}, id{id}, mask{io_uring_buf_ring_mask(bufferNumber)}, offset{0},
      ring{ring} {
    int result{0};
    this->self = io_uring_setup_buf_ring(&this->ring->get(), this->bufferNumber, id, 0, &result);
    if (this->self == nullptr)
        throw runtime_error("bufferRing setup error: " + string{std::strerror(std::abs(result))});

    io_uring_buf_ring_init(this->self);

    this->allocBuffers();
}

BufferRing::BufferRing(BufferRing &&other) noexcept
    : self{other.self}, buffers{other.buffers}, bufferNumber{other.bufferNumber}, bufferSize{other.bufferSize},
      id{other.id}, mask{other.mask}, offset{other.offset}, ring{std::move(other.ring)} {
    other.self = nullptr;
}

auto BufferRing::operator=(BufferRing &&other) noexcept -> BufferRing & {
    if (this != &other) {
        this->self = other.self;
        this->buffers = other.buffers;
        this->bufferNumber = other.bufferNumber;
        this->bufferSize = other.bufferSize;
        this->id = other.id;
        this->mask = other.mask;
        this->offset = other.offset;
        this->ring = std::move(other.ring);
        other.self = nullptr;
    }
    return *this;
}

auto BufferRing::getId() const -> int { return this->id; }

auto BufferRing::getData(unsigned short index, unsigned int size) -> string {
    string data{this->buffers[index], this->buffers[index] + size};

    io_uring_buf_ring_add(this->self, this->buffers[index], this->bufferSize, index, this->mask, this->offset++);

    return data;
}

auto BufferRing::advance(int completionNumber) -> void {
    __io_uring_buf_ring_cq_advance(&this->ring->get(), this->self, completionNumber, this->offset);

    this->offset = 0;
}

BufferRing::~BufferRing() {
    if (this->self != nullptr) {
        int returnValue{io_uring_free_buf_ring(&this->ring->get(), this->self, this->bufferNumber, this->id)};
        if (returnValue < 0)
            Log::add(source_location::current(), Level::ERROR,
                     "free bufferRing error: " + string{std::strerror(std::abs(returnValue))});

        for (unsigned int i{0}; i < this->bufferNumber; ++i) std::free(this->buffers[i]);

        std::free(this->buffers);
    }
}

auto BufferRing::allocBuffers() -> void {
    for (unsigned int i{0}; i < this->bufferNumber; ++i) {
        buffers[i] = static_cast<char *>(aligned_alloc(sysconf(_SC_PAGESIZE), this->bufferSize * sizeof(char)));
        io_uring_buf_ring_add(this->self, buffers[i], this->bufferSize, i, this->mask, static_cast<int>(i));
    }

    io_uring_buf_ring_advance(this->self, static_cast<int>(this->bufferNumber));
}
