#include "UserRing.h"

#include <sys/resource.h>

#include <exception>

using namespace std;

auto UserRing::getFileDescriptorLimit() noexcept -> unsigned int {
    rlimit limit{};

    const signed char result{static_cast<signed char>(getrlimit(RLIMIT_NOFILE, &limit))};
    if (result != 0) terminate();

    return limit.rlim_cur;
}

UserRing::UserRing(unsigned int entries, io_uring_params &params) noexcept : userRing{} {
    const short result{static_cast<short>(io_uring_queue_init_params(entries, &this->userRing, &params))};

    if (result != 0) terminate();
}

UserRing::UserRing(UserRing &&other) noexcept : userRing{other.userRing} { other.userRing.ring_fd = -1; }

auto UserRing::getSelfFileDescriptor() const noexcept -> unsigned int { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor() noexcept -> void {
    const short result{static_cast<short>(io_uring_register_ring_fd(&this->userRing))};
    if (result != 1) terminate();
}

auto UserRing::registerCpu(unsigned short cpuCode) noexcept -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    const signed char result{
            static_cast<signed char>(io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet))};
    if (result != 0) terminate();
}

auto UserRing::registerSparseFileDescriptors(unsigned int fileDescriptorCount) noexcept -> void {
    const int result{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (result != 0) terminate();
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length) noexcept -> void {
    const short result{static_cast<short>(io_uring_register_file_alloc_range(&this->userRing, offset, length))};
    if (result != 0) terminate();
}

auto UserRing::updateFileDescriptors(unsigned int offset, span<const unsigned int> fileDescriptors) noexcept -> void {
    const int result{io_uring_register_files_update(
            &this->userRing, offset, reinterpret_cast<const int *>(fileDescriptors.data()), fileDescriptors.size())};
    if (result < 0) terminate();
}

auto UserRing::setupBufferRing(unsigned short entries, unsigned short id) noexcept -> io_uring_buf_ring * {
    int result;

    io_uring_buf_ring *const bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, 0, &result)};
    if (bufferRing == nullptr) terminate();

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) noexcept
        -> void {
    const short result{static_cast<short>(io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id))};
    if (result < 0) terminate();
}

auto UserRing::submitWait(unsigned int count) noexcept -> void {
    const int result{io_uring_submit_and_wait(&this->userRing, count)};
    if (result < 0) terminate();
}

auto UserRing::forEachCompletion(const function<auto(io_uring_cqe *cqe)->void> &task) noexcept -> unsigned int {
    unsigned int completionCount{0}, head;

    io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->userRing, head, cqe) {
        task(cqe);
        ++completionCount;
    }

    return completionCount;
}

auto UserRing::getSqe() noexcept -> io_uring_sqe * {
    io_uring_sqe *const sqe{io_uring_get_sqe(&this->userRing)};
    if (sqe == nullptr) terminate();

    return sqe;
}

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                                 unsigned short bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, static_cast<int>(completionCount),
                                   bufferRingBufferCount);
}

UserRing::~UserRing() {
    if (this->userRing.ring_fd != -1) io_uring_queue_exit(&this->userRing);
}
