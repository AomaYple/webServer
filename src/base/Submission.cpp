#include "Submission.h"

Submission::Submission(io_uring_sqe *sqe) noexcept : submission{sqe} {}

auto Submission::setUserData(unsigned long long userData) noexcept -> void {
    io_uring_sqe_set_data64(this->submission, userData);
}

auto Submission::setFlags(unsigned int flags) noexcept -> void { io_uring_sqe_set_flags(this->submission, flags); }

auto Submission::setBufferGroup(unsigned short bufferGroup) noexcept -> void {
    this->submission->buf_group = bufferGroup;
}

auto Submission::accept(int fileDescriptor, sockaddr *address, socklen_t *addressLength, int flags) noexcept -> void {
    io_uring_prep_multishot_accept_direct(this->submission, fileDescriptor, address, addressLength, flags);
}

auto Submission::read(int fileDescriptor, void *buffer, unsigned int bufferLength,
                      unsigned long long int offset) noexcept -> void {
    io_uring_prep_read(this->submission, fileDescriptor, buffer, bufferLength, offset);
}

auto Submission::receive(int fileDescriptor, void *buffer, unsigned long bufferLength, int flags) noexcept -> void {
    io_uring_prep_recv_multishot(this->submission, fileDescriptor, buffer, bufferLength, flags);
}

auto Submission::send(int fileDescriptor, const void *buffer, unsigned long bufferLength, int flags,
                      unsigned int zeroCopyFlags) noexcept -> void {
    io_uring_prep_send_zc(this->submission, fileDescriptor, buffer, bufferLength, flags, zeroCopyFlags);
}

auto Submission::cancel(int fileDescriptor, int flags) noexcept -> void {
    io_uring_prep_cancel_fd(this->submission, fileDescriptor, flags);
}

auto Submission::close(int fileDescriptorIndex) noexcept -> void {
    io_uring_prep_close_direct(this->submission, fileDescriptorIndex);
}
