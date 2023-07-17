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
    int returnValue{io_uring_queue_init_params(entries, &this->userRing, &params)};
    if (returnValue != 0)
        throw runtime_error("userRing initialize error: " + string{std::strerror(std::abs(returnValue))});
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
    int returnValue{io_uring_register_ring_fd(&this->userRing)};
    if (returnValue != 1)
        throw runtime_error("userRing register self file descriptor error: " +
                            string{std::strerror(std::abs(returnValue))});
}

auto UserRing::registerCpu(unsigned short cpuCode) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    int returnValue{io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet)};
    if (returnValue != 0)
        throw runtime_error("userRing register cpu error: " + string{std::strerror(std::abs(returnValue))});
}

auto UserRing::registerFileDescriptors(unsigned int fileDescriptorCount) -> void {
    int returnValue{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (returnValue != 0)
        throw runtime_error("userRing register file descriptors error: " +
                            string{std::strerror(std::abs(returnValue))});
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length) -> void {
    int returnValue{io_uring_register_file_alloc_range(&this->userRing, offset, length)};
    if (returnValue != 0)
        throw runtime_error("userRing allocate file descriptor range error: " +
                            string{std::strerror(std::abs(returnValue))});
}

auto UserRing::updateFileDescriptors(unsigned int offset, span<int> fileDescriptors) -> void {
    int returnValue{
            io_uring_register_files_update(&this->userRing, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (returnValue < 0)
        throw runtime_error("userRing update file descriptors error: " + string{std::strerror(std::abs(returnValue))});
}

auto UserRing::setupBufferRing(unsigned short entries, unsigned short id) -> io_uring_buf_ring * {
    int returnValue;

    io_uring_buf_ring *bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, 0, &returnValue)};
    if (bufferRing == nullptr)
        throw runtime_error("userRing setup bufferRing error: " + string{std::strerror(std::abs(returnValue))});

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id) -> void {
    int returnValue{io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id)};
    if (returnValue < 0)
        throw runtime_error("userRing free bufferRing error: " + string{std::strerror(std::abs(returnValue))});
}

auto UserRing::submitWait(unsigned int count) -> void {
    int returnValue{io_uring_submit_and_wait(&this->userRing, count)};
    if (returnValue < 0)
        throw runtime_error("userRing submit wait error: " + string{std::strerror(std::abs(returnValue))});
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

auto UserRing::getSubmission() -> io_uring_sqe * {
    io_uring_sqe *submission{io_uring_get_sqe(&this->userRing)};
    if (submission == nullptr) throw runtime_error("userRing no available submission");

    return submission;
}

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                                 unsigned short bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, static_cast<int>(completionCount),
                                   bufferRingBufferCount);
}

UserRing::~UserRing() {
    if (this->userRing.ring_fd != -1) io_uring_queue_exit(&this->userRing);
}
