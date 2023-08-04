#include "UserRing.h"

#include "../exception/Exception.h"

#include <sys/resource.h>

#include <cstring>

using std::function;
using std::source_location;
using std::span;

auto getFileDescriptorLimit(std::source_location sourceLocation) -> std::uint_fast64_t {
    rlimit limit{};

    std::int_fast32_t returnValue{getrlimit(RLIMIT_NOFILE, &limit)};
    if (returnValue != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};

    return limit.rlim_cur;
}

UserRing::UserRing(std::uint_fast32_t entries, io_uring_params &params, source_location sourceLocation) : userRing{} {
    std::int_fast32_t result{io_uring_queue_init_params(entries, &this->userRing, &params)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::getSelfFileDescriptor() const noexcept -> std::int_fast32_t { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor(source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_register_ring_fd(&this->userRing)};
    if (result != 1) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::registerCpu(std::uint_fast16_t cpuCode, source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    std::int_fast32_t result{io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::registerFileDescriptors(std::uint_fast32_t fileDescriptorCount, source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::allocateFileDescriptorRange(std::uint_fast32_t offset, std::uint_fast32_t length,
                                           source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_register_file_alloc_range(&this->userRing, offset, length)};
    if (result != 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::updateFileDescriptors(std::uint_fast32_t offset, span<std::int_fast32_t> fileDescriptors,
                                     source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_register_files_update(
            &this->userRing, offset, reinterpret_cast<const int *>(fileDescriptors.data()), fileDescriptors.size())};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::setupBufferRing(std::uint_fast16_t entries, std::uint_fast16_t id, source_location sourceLocation)
        -> io_uring_buf_ring * {
    std::int_fast32_t result;

    io_uring_buf_ring *bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, static_cast<int>(id), 0,
                                                          reinterpret_cast<int *>(&result))};
    if (bufferRing == nullptr)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, std::uint_fast16_t entries, std::uint_fast16_t id,
                              source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_free_buf_ring(&this->userRing, bufferRing, entries, static_cast<int>(id))};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::submitWait(std::uint_fast32_t count, source_location sourceLocation) -> void {
    std::int_fast32_t result{io_uring_submit_and_wait(&this->userRing, count)};
    if (result < 0) throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}

auto UserRing::forEachCompletion(const function<auto(io_uring_cqe *cqe)->void> &task) -> std::uint_fast32_t {
    std::uint_fast32_t head, completionCount{0};

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

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, std::uint_fast32_t completionCount,
                                                 std::uint_fast16_t bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, static_cast<int>(completionCount),
                                   static_cast<int>(bufferRingBufferCount));
}

UserRing::~UserRing() { io_uring_queue_exit(&this->userRing); }
