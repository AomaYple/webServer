#include "Submission.h"

using namespace std;

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, sockaddr *address, socklen_t *addressLength,
                       unsigned int flags) noexcept
    : submission{sqe} {
    io_uring_prep_multishot_accept_direct(this->submission, static_cast<int>(fileDescriptor), address, addressLength,
                                          static_cast<int>(flags));
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, span<byte> buffer, unsigned long offset) noexcept
    : submission{sqe} {
    io_uring_prep_read(this->submission, static_cast<int>(fileDescriptor), buffer.data(), buffer.size(), offset);
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, span<byte> buffer, unsigned int flags) noexcept
    : submission{sqe} {
    io_uring_prep_recv_multishot(this->submission, static_cast<int>(fileDescriptor), buffer.data(), buffer.size(),
                                 static_cast<int>(flags));
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, span<const byte> buffer, unsigned int flags,
                       unsigned char zeroCopyFlags) noexcept
    : submission{sqe} {
    io_uring_prep_send_zc(this->submission, static_cast<int>(fileDescriptor), buffer.data(), buffer.size(),
                          static_cast<int>(flags), zeroCopyFlags);
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptor, unsigned char flags) noexcept : submission{sqe} {
    io_uring_prep_cancel_fd(this->submission, static_cast<int>(fileDescriptor), flags);
}

Submission::Submission(io_uring_sqe *sqe, unsigned int fileDescriptorIndex) noexcept : submission{sqe} {
    io_uring_prep_close_direct(this->submission, fileDescriptorIndex);
}

auto Submission::setUserData(unsigned long userData) const noexcept -> void {
    io_uring_sqe_set_data64(this->submission, userData);
}

auto Submission::setFlags(unsigned char flags) const noexcept -> void {
    io_uring_sqe_set_flags(this->submission, flags);
}

auto Submission::setBufferRingId(unsigned short bufferRingId) const noexcept -> void {
    this->submission->buf_group = bufferRingId;
}
