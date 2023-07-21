#include "UserRing.h"

#include <cstring>
#include <sys/resource.h>

#include <stdexcept>

using std::runtime_error;
using std::string, std::span, std::function;

auto getFileDescriptorLimit() -> unsigned int {
    rlimit limit{};

    int returnValue{getrlimit(RLIMIT_NOFILE, &limit)};
    if (returnValue != 0) throw runtime_error("get file descriptor limit error: " + string{std::strerror(errno)});

    return limit.rlim_cur;
}

UserRing::UserRing(unsigned int entries, io_uring_params &params) : userRing{} {
    int result{io_uring_queue_init_params(entries, &this->userRing, &params)};
    if (result != 0) throw runtime_error("userRing initialize error: " + string{std::strerror(std::abs(result))});
}

UserRing::UserRing(UserRing &&other) noexcept : userRing{other.userRing} { other.userRing.ring_fd = -1; }

auto UserRing::operator=(UserRing &&other) noexcept -> UserRing & {
    if (this != &other) {
        this->userRing = other.userRing;
        other.userRing.ring_fd = -1;
    }
    return *this;
}

auto UserRing::getSelfFileDescriptor() const noexcept -> int { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor() -> void {
    int result{io_uring_register_ring_fd(&this->userRing)};
    if (result != 1)
        throw runtime_error("userRing register self file descriptor error: " + string{std::strerror(std::abs(result))});
}

auto UserRing::registerCpu(unsigned short cpuCode) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    int result{io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet)};
    if (result != 0) throw runtime_error("userRing register cpu error: " + string{std::strerror(std::abs(result))});
}

auto UserRing::registerFileDescriptors(unsigned int fileDescriptorCount) -> void {
    int result{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (result != 0)
        throw runtime_error("userRing register file descriptors error: " + string{std::strerror(std::abs(result))});
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length) -> void {
    int result{io_uring_register_file_alloc_range(&this->userRing, offset, length)};
    if (result != 0)
        throw runtime_error("userRing allocate file descriptor range error: " +
                            string{std::strerror(std::abs(result))});
}

auto UserRing::updateFileDescriptors(unsigned int offset, span<int> fileDescriptors) -> void {
    int result{io_uring_register_files_update(&this->userRing, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (result < 0)
        throw runtime_error("userRing update file descriptors error: " + string{std::strerror(std::abs(result))});
}

auto UserRing::setupBufferRing(unsigned short entries, unsigned short id) -> io_uring_buf_ring * {
    int result;

    io_uring_buf_ring *bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, 0, &result)};
    if (bufferRing == nullptr)
        throw runtime_error("userRing setup bufferRing error: " + string{std::strerror(std::abs(result))});

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) -> void {
    int result{io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id)};
    if (result < 0) throw runtime_error("userRing free bufferRing error: " + string{std::strerror(std::abs(result))});
}

auto UserRing::submitWait(unsigned int count) -> void {
    int result{io_uring_submit_and_wait(&this->userRing, count)};
    if (result < 0) throw runtime_error("userRing submit wait error: " + string{std::strerror(std::abs(result))});
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

auto UserRing::getSqe() -> io_uring_sqe * {
    io_uring_sqe *sqe{io_uring_get_sqe(&this->userRing)};
    if (sqe == nullptr) throw runtime_error("userRing no available sqe");

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
