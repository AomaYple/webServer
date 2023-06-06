#include "Submission.h"

#include <stdexcept>

#include "Ring.h"

using std::runtime_error;

Submission::Submission(Ring& ring) : self{ring.getSubmission()}, used{false} {}

Submission::Submission(Submission&& submission) noexcept : self{submission.self}, used{submission.used} {}

auto Submission::operator=(Submission&& submission) noexcept -> Submission& {
    if (this != &submission) {
        this->self = submission.self;
        this->used = submission.used;
    }
    return *this;
}

auto Submission::setData(void* data) -> void { io_uring_sqe_set_data(this->self, data); }

auto Submission::setFlags(unsigned int flags) -> void { io_uring_sqe_set_flags(this->self, flags); }

auto Submission::setBufferId(unsigned short id) -> void { this->self->buf_group = id; }

auto Submission::time(__kernel_timespec* time, unsigned int count, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout(this->self, time, count, flags);
}

auto Submission::updateTime(__kernel_timespec* time, unsigned long long data, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout_update(this->self, time, data, flags);
}

auto Submission::removeTime(unsigned long long int data, unsigned int flags) -> void {
    this->judgeUsed();

    io_uring_prep_timeout_remove(this->self, data, flags);
}

auto Submission::accept(int socket, sockaddr* address, socklen_t* addressLength, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_multishot_accept_direct(this->self, socket, address, addressLength, flags);
}

auto Submission::receive(int socket, void* buffer, unsigned long length, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_recv_multishot(this->self, socket, buffer, length, flags);
}

auto Submission::send(int socket, const void* buffer, unsigned long length, int flags, int zeroCopyFlags) -> void {
    this->judgeUsed();

    io_uring_prep_send_zc(this->self, socket, buffer, length, flags, zeroCopyFlags);
}

auto Submission::close(int fileIndex) -> void {
    this->judgeUsed();

    io_uring_prep_close_direct(this->self, fileIndex);
}

auto Submission::cancel(void* data, int flags) -> void {
    this->judgeUsed();

    io_uring_prep_cancel(this->self, data, flags);
}

auto Submission::judgeUsed() -> void {
    if (this->used) throw runtime_error("submission is used");

    this->used = true;
}
