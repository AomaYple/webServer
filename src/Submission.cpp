#include "Submission.h"

#include <stdexcept>

#include "Ring.h"

using std::runtime_error;

Submission::Submission(Ring &ring) : self{ring.getSubmission()}, used{false} {}

Submission::Submission(Submission &&other) noexcept : self{other.self}, used{other.used} {}

auto Submission::operator=(Submission &&other) noexcept -> Submission & {
    if (this != &other) {
        this->self = other.self;
        this->used = other.used;
    }
    return *this;
}

auto Submission::setData(unsigned long long data) -> void { io_uring_sqe_set_data64(this->self, data); }

auto Submission::setFlags(unsigned int flags) -> void { io_uring_sqe_set_flags(this->self, flags); }

auto Submission::setBufferGroup(unsigned short id) -> void { this->self->buf_group = id; }

auto Submission::accept(int fileDescriptor, sockaddr *address, socklen_t *addressLength, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_multishot_accept_direct(this->self, fileDescriptor, address, addressLength, flags);
}

auto Submission::receive(int fileDescriptor, void *buffer, unsigned long length, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_recv_multishot(this->self, fileDescriptor, buffer, length, flags);
}

auto Submission::close(int fileDescriptor) -> void {
    this->judgeUsed();

    io_uring_prep_close_direct(this->self, fileDescriptor);
}

auto Submission::timeout(__kernel_timespec *timespec, unsigned int count, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout(this->self, timespec, count, flags);
}

auto Submission::updateTimeout(__kernel_timespec *timespec, unsigned long long int data, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout_update(this->self, timespec, data, flags);
}

auto Submission::removeTimeout(unsigned long long int data, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout_remove(this->self, data, flags);
}

auto Submission::cancelFileDescriptor(int fileDescriptor, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_cancel_fd(this->self, fileDescriptor, flags);
}

auto Submission::judgeUsed() -> void {
    if (this->used) throw runtime_error("submission is used");
    this->used = true;
}
