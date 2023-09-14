#include "Submission.hpp"

Submission::Submission(io_uring_sqe *sqe, int fileDescriptor, sockaddr *address, socklen_t *addressLength,
                       int flags) noexcept
    : submission{sqe} {
    io_uring_prep_multishot_accept_direct(this->submission, fileDescriptor, address, addressLength, flags);
}

Submission::Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<std::byte> buffer,
                       unsigned long offset) noexcept
    : submission{sqe} {
    io_uring_prep_read(this->submission, fileDescriptor, buffer.data(), buffer.size_bytes(), offset);
}

Submission::Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<std::byte> buffer, int flags) noexcept
    : submission{sqe} {
    io_uring_prep_recv_multishot(this->submission, fileDescriptor, buffer.data(), buffer.size_bytes(), flags);
}

Submission::Submission(io_uring_sqe *sqe, int fileDescriptor, std::span<const std::byte> buffer, int flags,
                       unsigned int zeroCopyFlags) noexcept
    : submission{sqe} {
    io_uring_prep_send_zc(this->submission, fileDescriptor, buffer.data(), buffer.size_bytes(), flags, zeroCopyFlags);
}

Submission::Submission(io_uring_sqe *sqe, int fileDescriptor, unsigned int flags) noexcept : submission{sqe} {
    io_uring_prep_cancel_fd(this->submission, fileDescriptor, flags);
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptorIndex) noexcept : submission{sqe} {
    io_uring_prep_close_direct(this->submission, fileDescriptorIndex);
}

auto Submission::setUserData(unsigned long userData) const noexcept -> void {
    io_uring_sqe_set_data64(this->submission, userData);
}

auto Submission::setFlags(unsigned int flags) const noexcept -> void {
    io_uring_sqe_set_flags(this->submission, flags);
}

auto Submission::setBufferRingId(unsigned short bufferRingId) const noexcept -> void {
    this->submission->buf_group = bufferRingId;
}