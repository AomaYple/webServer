#include "Buffer.h"

#include "Log.h"

using std::vector, std::string, std::shared_ptr, std::aligned_alloc, std::runtime_error, std::source_location;

thread_local bool Buffer::instance{false};

constexpr unsigned short bufferNumber{1024}, bufferSize{1024}, bufferId{0};

Buffer::Buffer(shared_ptr<Ring> &ioUserRing)
    : self{static_cast<io_uring_buf_ring *>(
          aligned_alloc(sysconf(_SC_PAGESIZE), bufferNumber * sizeof(io_uring_buf_ring)))},
      buffers{bufferNumber, vector<char>(bufferSize, 0)},
      id{bufferId},
      offset{0},
      mask{io_uring_buf_ring_mask(bufferNumber)},
      ring{ioUserRing} {
    if (Buffer::instance) throw runtime_error("buffer one thread one instance");
    Buffer::instance = true;

    if (this->self == nullptr) throw runtime_error("buffer allocate memory failed");

    io_uring_buf_reg reg{reinterpret_cast<unsigned long long>(this->self),
                         static_cast<unsigned int>(this->buffers.size()), this->id};

    this->ring->registerBuffer(reg);

    io_uring_buf_ring_init(this->self);

    for (unsigned int i{0}; i < this->buffers.size(); ++i)
        io_uring_buf_ring_add(this->self, this->buffers[i].data(), this->buffers[i].size(), i, this->mask,
                              static_cast<int>(i));

    io_uring_buf_ring_advance(this->self, static_cast<int>(this->buffers.size()));
}

Buffer::Buffer(Buffer &&buffer) noexcept
    : self{buffer.self},
      buffers{std::move(buffer.buffers)},
      id{buffer.id},
      offset{buffer.offset},
      mask{buffer.mask},
      ring{std::move(buffer.ring)} {
    buffer.self = nullptr;
}

auto Buffer::operator=(Buffer &&buffer) noexcept -> Buffer & {
    if (this != &buffer) {
        this->self = buffer.self;
        this->buffers = std::move(buffer.buffers);
        this->id = buffer.id;
        this->offset = buffer.offset;
        this->mask = buffer.mask;
        this->ring = std::move(buffer.ring);
        buffer.self = nullptr;
    }
    return *this;
}

auto Buffer::getId() const -> unsigned short { return this->id; }

auto Buffer::getData(unsigned short index, unsigned long size) -> string {
    string data{this->buffers[index].begin(), this->buffers[index].begin() + static_cast<unsigned short>(size)};

    io_uring_buf_ring_add(this->self, this->buffers[index].data(), this->buffers[index].size(), index, this->mask,
                          this->offset++);

    return data;
}

auto Buffer::advanceBufferCompletion(int number) -> void {
    this->ring->advanceBufferCompletion(this->self, number);

    this->offset = 0;
}

Buffer::~Buffer() {
    if (this->self != nullptr) {
        try {
            ring->unregisterBuffer(this->id);
        } catch (runtime_error &runtimeError) {
            Log::add(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }

    std::free(this->self);

    Buffer::instance = false;
}
