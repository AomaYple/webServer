#include "UserRing.hpp"

#include "../log/Log.hpp"
#include "UserRingCallError.hpp"

#include <sys/resource.h>

#include <cstring>

UserRing::UserRing(unsigned int entries, io_uring_params &params, std::source_location sourceLocation) : userRing{} {
    const short result{static_cast<short>(io_uring_queue_init_params(entries, &this->userRing, &params))};

    if (result != 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

UserRing::UserRing(UserRing &&other) noexcept : userRing{other.userRing} { other.userRing.ring_fd = -1; }

auto UserRing::operator=(UserRing &&other) noexcept -> UserRing & {
    if (this != &other) {
        this->destroy();

        this->userRing = other.userRing;
        other.userRing.ring_fd = -1;
    }

    return *this;
}

UserRing::~UserRing() { this->destroy(); }

auto UserRing::destroy() noexcept -> void {
    if (this->userRing.ring_fd != -1) io_uring_queue_exit(&this->userRing);
}

auto UserRing::getFileDescriptorLimit(std::source_location sourceLocation) -> unsigned int {
    rlimit limit{};

    if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation, std::strerror(errno))};

    return limit.rlim_cur;
}

auto UserRing::getSelfFileDescriptor() const noexcept -> unsigned int { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor(std::source_location sourceLocation) -> void {
    const short result{static_cast<short>(io_uring_register_ring_fd(&this->userRing))};
    if (result != 1)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::registerCpu(unsigned short cpuCode, std::source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    const signed char result{
            static_cast<signed char>(io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet))};
    if (result != 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::registerSparseFileDescriptors(unsigned int fileDescriptorCount, std::source_location sourceLocation)
        -> void {
    const short result{static_cast<short>(io_uring_register_files_sparse(&this->userRing, fileDescriptorCount))};
    if (result != 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length,
                                           std::source_location sourceLocation) -> void {
    const short result{static_cast<short>(io_uring_register_file_alloc_range(&this->userRing, offset, length))};
    if (result != 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::updateFileDescriptors(unsigned int offset, std::span<const unsigned int> fileDescriptors,
                                     std::source_location sourceLocation) -> void {
    const std::vector<int> realFileDescriptors{fileDescriptors.cbegin(), fileDescriptors.cend()};

    const int result{io_uring_register_files_update(&this->userRing, offset, realFileDescriptors.data(),
                                                    realFileDescriptors.size())};
    if (result < 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::setupBufferRing(unsigned short entries, unsigned short id, std::source_location sourceLocation)
        -> io_uring_buf_ring * {
    int result;

    io_uring_buf_ring *const bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, 0, &result)};
    if (bufferRing == nullptr)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned short entries, unsigned short id,
                              std::source_location sourceLocation) -> void {
    const short result{static_cast<short>(io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id))};
    if (result < 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::submitWait(unsigned int waitCount, std::source_location sourceLocation) -> void {
    const int result{io_uring_submit_and_wait(&this->userRing, waitCount)};
    if (result < 0)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation,
                                               std::strerror(std::abs(result)))};
}

auto UserRing::forEachCompletion(const std::function<auto(const io_uring_cqe *cqe)->void> &task) noexcept
        -> unsigned int {
    unsigned int completionCount{0}, head;

    const io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->userRing, head, cqe) {
        task(cqe);
        ++completionCount;
    }

    return completionCount;
}

auto UserRing::getSqe(std::source_location sourceLocation) -> io_uring_sqe * {
    io_uring_sqe *const sqe{io_uring_get_sqe(&this->userRing)};
    if (sqe == nullptr)
        throw UserRingCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                               std::this_thread::get_id(), sourceLocation, "no sqe available")};

    return sqe;
}

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, unsigned int completionCount,
                                                 unsigned short bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, static_cast<int>(completionCount),
                                   bufferRingBufferCount);
}
