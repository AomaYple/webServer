#include "UserRing.h"

#include <cstring>

#include <sys/resource.h>

#include "../exception/Exception.h"

using std::function;
using std::runtime_error;
using std::source_location;
using std::span;
using std::string;

auto getFileDescriptorLimit(source_location sourceLocation) -> unsigned int {
    rlimit limit{};

    int returnValue{getrlimit(RLIMIT_NOFILE, &limit)};
    if (returnValue != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};

    return limit.rlim_cur;
}

UserRing::UserRing(unsigned int entries, io_uring_params &params, source_location sourceLocation) : userRing{} {
    int result{io_uring_queue_init_params(entries, &this->userRing, &params)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::getSelfFileDescriptor() const noexcept -> int { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor(source_location sourceLocation) -> void {
    int result{io_uring_register_ring_fd(&this->userRing)};
    if (result != 1) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::registerCpu(unsigned short cpuCode, source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    int result{io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::registerFileDescriptors(unsigned int fileDescriptorCount, source_location sourceLocation) -> void {
    int result{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length, source_location sourceLocation)
        -> void {
    int result{io_uring_register_file_alloc_range(&this->userRing, offset, length)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::updateFileDescriptors(unsigned int offset, span<int> fileDescriptors, source_location sourceLocation)
        -> void {
    int result{io_uring_register_files_update(&this->userRing, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::setupBufferRing(unsigned short entries, unsigned short id, source_location sourceLocation)
        -> io_uring_buf_ring * {
    int result;

    io_uring_buf_ring *bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, 0, &result)};
    if (bufferRing == nullptr) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id,
                              source_location sourceLocation) -> void {
    int result{io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id)};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::submitWait(unsigned int count, source_location sourceLocation) -> void {
    int result{io_uring_submit_and_wait(&this->userRing, count)};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}

auto UserRing::forEachCompletion(const function<auto(io_uring_cqe *cqe)->void> &task) -> unsigned int {
    unsigned int head, completionCount{0};

    io_uring_cqe *completion;
    io_uring_for_each_cqe(&this->userRing, head, completion) {
        task(completion);
        ++completionCount;
    }

    return completionCount;
}

auto UserRing::getSqe(source_location sourceLocation) -> io_uring_sqe * {
    io_uring_sqe *sqe{io_uring_get_sqe(&this->userRing)};
    if (sqe == nullptr) throw Exception{sourceLocation, Level::FATAL, "no sqe available"};

    return sqe;
}

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                                 unsigned short bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, static_cast<int>(completionCount),
                                   bufferRingBufferCount);
}

UserRing::~UserRing() { io_uring_queue_exit(&this->userRing); }
