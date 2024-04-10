#include "Ring.hpp"

#include "../log/Exception.hpp"
#include "Completion.hpp"
#include "Submission.hpp"

#include <sys/resource.h>

#include <cstring>

auto Ring::getFileDescriptorLimit(std::source_location sourceLocation) -> unsigned long {
    rlimit limit{};
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    return limit.rlim_cur;
}

Ring::Ring(unsigned int entries, io_uring_params &params) : handle{Ring::initialize(entries, params)} {}

Ring::Ring(Ring &&other) noexcept : handle{other.handle} { other.handle.ring_fd = -1; }

auto Ring::operator=(Ring &&other) noexcept -> Ring & {
    this->destroy();

    this->handle = other.handle;
    other.handle.ring_fd = -1;

    return *this;
}

Ring::~Ring() { this->destroy(); }

auto Ring::getFileDescriptor() const noexcept -> int { return this->handle.ring_fd; }

auto Ring::registerSelfFileDescriptor(std::source_location sourceLocation) -> void {
    const int result{io_uring_register_ring_fd(&this->handle)};
    if (result != 1) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::registerCpu(unsigned short cpuCode, std::source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};
    CPU_SET(cpuCode, &cpuSet);

    const int result{io_uring_register_iowq_aff(&this->handle, sizeof(cpuSet), &cpuSet)};
    if (result != 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::registerSparseFileDescriptor(unsigned int count, std::source_location sourceLocation) -> void {
    const int result{io_uring_register_files_sparse(&this->handle, count)};
    if (result != 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::allocateFileDescriptorRange(unsigned int offset, unsigned int length, std::source_location sourceLocation)
        -> void {
    const int result{io_uring_register_file_alloc_range(&this->handle, offset, length)};
    if (result != 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::updateFileDescriptors(unsigned int offset, std::span<const int> fileDescriptors,
                                 std::source_location sourceLocation) -> void {
    const int result{
            io_uring_register_files_update(&this->handle, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (result < 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::setupRingBuffer(unsigned int entries, int id, std::source_location sourceLocation) -> io_uring_buf_ring * {
    int result;
    io_uring_buf_ring *const ringBufferHandle{io_uring_setup_buf_ring(&this->handle, entries, id, 0, &result)};
    if (ringBufferHandle == nullptr)
        throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};

    return ringBufferHandle;
}

auto Ring::freeRingBuffer(io_uring_buf_ring *ringBufferHandle, unsigned int entries, int id,
                          std::source_location sourceLocation) -> void {
    const int result{io_uring_free_buf_ring(&this->handle, ringBufferHandle, entries, id)};
    if (result < 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::submit(const Submission &submission) -> void {
    io_uring_sqe *const sqe{this->getSqe()};

    const int fileDescriptor{submission.getFileDescriptor()};
    const std::variant<Submission::Accept, Submission::Read, Submission::Receive, Submission::Send, Submission::Close>
            &parameter{submission.getParameter()};

    switch (parameter.index()) {
        case 0: {
            const Submission::Accept &acceptParameter{std::get<0>(parameter)};
            io_uring_prep_multishot_accept_direct(sqe, fileDescriptor, acceptParameter.address,
                                                  acceptParameter.addressLength, acceptParameter.flags);

            break;
        }
        case 1: {
            const Submission::Read &readParameter{std::get<1>(parameter)};
            io_uring_prep_read(sqe, fileDescriptor, readParameter.buffer.data(), readParameter.buffer.size(),
                               readParameter.offset);

            break;
        }
        case 2: {
            const Submission::Receive &receiveParameter{std::get<2>(parameter)};
            io_uring_prep_recv_multishot(sqe, fileDescriptor, receiveParameter.buffer.data(),
                                         receiveParameter.buffer.size(), receiveParameter.flags);
            sqe->buf_group = receiveParameter.ringBufferId;

            break;
        }
        case 3: {
            const Submission::Send &sendParameter{std::get<3>(parameter)};
            io_uring_prep_send_zc(sqe, fileDescriptor, sendParameter.buffer.data(), sendParameter.buffer.size(),
                                  sendParameter.flags, sendParameter.zeroCopyFlags);

            break;
        }
        case 4:
            io_uring_prep_close_direct(sqe, fileDescriptor);

            break;
    }

    io_uring_sqe_set_flags(sqe, submission.getFlags());
    io_uring_sqe_set_data(sqe, submission.getUserData());
}

auto Ring::poll(const std::function<auto(const Completion &completion)->void> &action) -> void {
    this->wait(1);

    int count{0};
    unsigned int head;

    const io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->handle, head, cqe) {
        action({{cqe->res, cqe->flags}, io_uring_cqe_get_data(cqe)});
        ++count;
    }

    this->advance(count);
}

auto Ring::initialize(unsigned int entries, io_uring_params &params, std::source_location sourceLocation) -> io_uring {
    io_uring handle{};

    const int result{io_uring_queue_init_params(entries, &handle, &params)};
    if (result != 0) throw Exception{Log{Log::Level::fatal, std::strerror(std::abs(result)), sourceLocation}};

    return handle;
}

auto Ring::destroy() noexcept -> void {
    if (this->handle.ring_fd != -1) io_uring_queue_exit(&this->handle);
}

auto Ring::getSqe(std::source_location sourceLocation) -> io_uring_sqe * {
    io_uring_sqe *const sqe{io_uring_get_sqe(&this->handle)};
    if (sqe == nullptr) throw Exception{Log{Log::Level::error, "no sqe available", sourceLocation}};

    return sqe;
}

auto Ring::wait(unsigned int count, std::source_location sourceLocation) -> void {
    const int result{io_uring_submit_and_wait(&this->handle, count)};
    if (result < 0) throw Exception{Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}};
}

auto Ring::advance(int count) noexcept -> void { io_uring_cq_advance(&this->handle, count); }
