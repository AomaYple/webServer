#include "Submission.h"

Submission::Submission(io_uring_sqe *sqe) noexcept : self{sqe} {}

Submission::Submission(Submission &&other) noexcept : self{other.self} {}

auto Submission::operator=(Submission &&other) noexcept -> Submission & {
    if (this != &other) this->self = other.self;
    return *this;
}

auto Submission::setUserData(unsigned long long userData) noexcept -> void {
    io_uring_sqe_set_data64(this->self, userData);
}

auto Submission::setFlags(unsigned int flags) noexcept -> void { io_uring_sqe_set_flags(this->self, flags); }

auto Submission::setBufferGroup(unsigned short bufferGroup) noexcept -> void { this->self->buf_group = bufferGroup; }

auto Submission::accept(int socket, sockaddr *address, socklen_t *addressLength, int flags) noexcept -> void {
    io_uring_prep_multishot_accept_direct(this->self, socket, address, addressLength, flags);
}

auto Submission::read(int fileDescriptor, void *buffer, unsigned int bytesNumber,
                      unsigned long long int offset) noexcept -> void {
    io_uring_prep_read(this->self, fileDescriptor, buffer, bytesNumber, offset);
}

auto Submission::receive(int socket, void *buffer, unsigned long bufferLength, int flags) noexcept -> void {
    io_uring_prep_recv_multishot(this->self, socket, buffer, bufferLength, flags);
}

auto Submission::send(int socket, const void *buffer, unsigned long bufferLength, int flags,
                      unsigned int zeroCopyFlags) noexcept -> void {
    io_uring_prep_send_zc(this->self, socket, buffer, bufferLength, flags, zeroCopyFlags);
}

auto Submission::cancel(int socket, int flags) noexcept -> void { io_uring_prep_cancel_fd(this->self, socket, flags); }

auto Submission::close(int socket) noexcept -> void { io_uring_prep_close_direct(this->self, socket); }
