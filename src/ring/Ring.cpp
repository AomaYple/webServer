#include "Ring.hpp"

#include "../log/Exception.hpp"
#include "Completion.hpp"
#include "Submission.hpp"

#include <cstring>
#include <sys/resource.h>

auto Ring::getFileDescriptorLimit(std::source_location sourceLocation) -> unsigned long {
    rlimit limit{};
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
        throw Exception{
            Log{Log::Level::fatal, std::strerror(errno), sourceLocation}
        };
    }

    return limit.rlim_cur;
}

Ring::Ring(unsigned int entries, io_uring_params &params) : handle{Ring::initialize(entries, params)} {}

Ring::Ring(Ring &&other) noexcept : handle{other.handle} { other.handle.ring_fd = -1; }

auto Ring::operator=(Ring &&other) noexcept -> Ring & {
    if (this == &other) return *this;

    this->destroy();

    this->handle = other.handle;
    other.handle.ring_fd = -1;

    return *this;
}

Ring::~Ring() { this->destroy(); }

auto Ring::getFileDescriptor() const noexcept -> int { return this->handle.ring_fd; }

auto Ring::registerSelfFileDescriptor(std::source_location sourceLocation) -> void {
    const int result{io_uring_register_ring_fd(&this->handle)};
    if (result != 1) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::registerCpu(unsigned short cpuCode, std::source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};
    CPU_SET(cpuCode, &cpuSet);

    const int result{io_uring_register_iowq_aff(&this->handle, sizeof(cpuSet), &cpuSet)};
    if (result != 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::registerSparseFileDescriptor(unsigned int count, std::source_location sourceLocation) -> void {
    const int result{io_uring_register_files_sparse(&this->handle, count)};
    if (result != 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::allocateFileDescriptorRange(unsigned int offset, unsigned int length, std::source_location sourceLocation)
    -> void {
    const int result{io_uring_register_file_alloc_range(&this->handle, offset, length)};
    if (result != 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::updateFileDescriptors(unsigned int offset, std::span<const int> fileDescriptors,
                                 std::source_location sourceLocation) -> void {
    const int result{
        io_uring_register_files_update(&this->handle, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (result < 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::setupRingBuffer(unsigned int entries, int id, std::source_location sourceLocation) -> io_uring_buf_ring * {
    int result{};
    io_uring_buf_ring *const ringBufferHandle{io_uring_setup_buf_ring(&this->handle, entries, id, 0, &result)};
    if (ringBufferHandle == nullptr) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }

    return ringBufferHandle;
}

auto Ring::freeRingBuffer(io_uring_buf_ring *ringBufferHandle, unsigned int entries, int id,
                          std::source_location sourceLocation) -> void {
    const int result{io_uring_free_buf_ring(&this->handle, ringBufferHandle, entries, id)};
    if (result < 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::submit(const Submission &submission) -> void {
    io_uring_sqe *const sqe{this->getSqe()};

    switch (submission.parameter.index()) {
        case 0:
            {
                const Submission::Accept &parameter{std::get<0>(submission.parameter)};
                io_uring_prep_multishot_accept_direct(sqe, submission.fileDescriptor, parameter.address,
                                                      parameter.addressLength, parameter.flags);

                break;
            }
        case 1:
            {
                const Submission::Open &parameter{std::get<1>(submission.parameter)};
                io_uring_prep_openat2_direct(sqe, parameter.directoryFileDescriptor, parameter.path.data(),
                                             parameter.flags, parameter.mode, parameter.fileDescriptorIndex);

                break;
            }
        case 3:
            {
                const Submission::Read &parameter{std::get<3>(submission.parameter)};
                io_uring_prep_read(sqe, submission.fileDescriptor, parameter.buffer.data(), parameter.buffer.size(),
                                   parameter.offset);

                break;
            }
        case 4:
            {
                const Submission::Receive &parameter{std::get<4>(submission.parameter)};
                io_uring_prep_recv_multishot(sqe, submission.fileDescriptor, parameter.buffer.data(),
                                             parameter.buffer.size(), parameter.flags);
                sqe->buf_group = parameter.ringBufferId;

                break;
            }
        case 5:
            {
                const Submission::Send &parameter{std::get<5>(submission.parameter)};
                io_uring_prep_send_zc(sqe, submission.fileDescriptor, parameter.buffer.data(), parameter.buffer.size(),
                                      parameter.flags, parameter.zeroCopyFlags);

                break;
            }
        case 6:
            {
                const Submission::Cancel &parameter{std::get<6>(submission.parameter)};
                io_uring_prep_cancel_fd(sqe, submission.fileDescriptor, parameter.flags);

                break;
            }
        case 7:
            io_uring_prep_close_direct(sqe, submission.fileDescriptor);

            break;
    }

    io_uring_sqe_set_flags(sqe, submission.flags);
    io_uring_sqe_set_data64(sqe, submission.userData);
}

auto Ring::poll(const std::function<auto(const Completion &completion)->void> &action) -> void {
    this->wait(1);

    int count{};
    unsigned int head;

    const io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->handle, head, cqe) {
        action(Completion{
            Outcome{cqe->res, cqe->flags},
            io_uring_cqe_get_data64(cqe)
        });
        ++count;
    }

    this->advance(count);
}

auto Ring::initialize(unsigned int entries, io_uring_params &params, std::source_location sourceLocation) -> io_uring {
    io_uring handle{};

    const int result{io_uring_queue_init_params(entries, &handle, &params)};
    if (result != 0) {
        throw Exception{
            Log{Log::Level::fatal, std::strerror(std::abs(result)), sourceLocation}
        };
    }

    return handle;
}

auto Ring::destroy() noexcept -> void {
    if (this->handle.ring_fd != -1) io_uring_queue_exit(&this->handle);
}

auto Ring::getSqe(std::source_location sourceLocation) -> io_uring_sqe * {
    io_uring_sqe *const sqe{io_uring_get_sqe(&this->handle)};
    if (sqe == nullptr) {
        throw Exception{
            Log{Log::Level::error, "no sqe available", sourceLocation}
        };
    }

    return sqe;
}

auto Ring::wait(unsigned int count, std::source_location sourceLocation) -> void {
    const int result{io_uring_submit_and_wait(&this->handle, count)};
    if (result < 0) {
        throw Exception{
            Log{Log::Level::error, std::strerror(std::abs(result)), sourceLocation}
        };
    }
}

auto Ring::advance(int count) noexcept -> void { io_uring_cq_advance(&this->handle, count); }
