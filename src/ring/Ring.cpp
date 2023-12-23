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

    switch (submission.event.type) {
        case Event::Type::accept: {
            const Submission::AcceptParameters &parameters{
                    std::get<Submission::AcceptParameters>(submission.parameters)};
            io_uring_prep_multishot_accept_direct(sqe, submission.event.fileDescriptor, parameters.address,
                                                  parameters.addressLength, parameters.flags);

            break;
        }
        case Event::Type::timing: {
            const Submission::ReadParameters &parameters{std::get<Submission::ReadParameters>(submission.parameters)};
            io_uring_prep_read(sqe, submission.event.fileDescriptor, parameters.buffer.data(), parameters.buffer.size(),
                               parameters.offset);

            break;
        }
        case Event::Type::receive: {
            const Submission::ReceiveParameters &parameters{
                    std::get<Submission::ReceiveParameters>(submission.parameters)};
            io_uring_prep_recv_multishot(sqe, submission.event.fileDescriptor, nullptr, 0, parameters.flags);
            sqe->buf_group = parameters.ringBufferId;

            break;
        }
        case Event::Type::send: {
            const Submission::SendParameters &parameters{std::get<Submission::SendParameters>(submission.parameters)};
            io_uring_prep_send_zc(sqe, submission.event.fileDescriptor, parameters.buffer.data(),
                                  parameters.buffer.size(), parameters.flags, parameters.zeroCopyFlags);

            break;
        }
        case Event::Type::close:
            io_uring_prep_close_direct(sqe, submission.event.fileDescriptor);

            break;
    }

    io_uring_sqe_set_data64(sqe, std::bit_cast<unsigned long>(submission.event));
    io_uring_sqe_set_flags(sqe, submission.flags);
}

auto Ring::traverseCompletion(const std::function<auto(const Completion &completion)->void> &task) -> void {
    this->wait(1);

    int count{0};
    unsigned int head;

    const io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->handle, head, cqe) {
        task({std::bit_cast<Event>(io_uring_cqe_get_data64(cqe)), cqe->res, cqe->flags});
        ++count;
    }

    this->advanceCompletion(count);
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

auto Ring::advanceCompletion(int count) noexcept -> void { io_uring_cq_advance(&this->handle, count); }
