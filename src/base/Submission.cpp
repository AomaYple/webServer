#include "Submission.h"

Submission::Submission(io_uring_sqe *sqe) noexcept : submission{sqe} {}

auto Submission::setUserData(std::uint_fast64_t userData) noexcept -> void {
    io_uring_sqe_set_data64(this->submission, userData);
}

auto Submission::setFlags(std::uint_fast32_t flags) noexcept -> void {
    io_uring_sqe_set_flags(this->submission, flags);
}

auto Submission::setBufferGroup(std::uint_fast16_t bufferGroup) noexcept -> void {
    this->submission->buf_group = bufferGroup;
}

auto Submission::accept(std::int_fast32_t fileDescriptor, sockaddr *address, socklen_t *addressLength,
                        std::int_fast32_t flags) noexcept -> void {
    io_uring_prep_multishot_accept_direct(this->submission, static_cast<int>(fileDescriptor), address, addressLength,
                                          static_cast<int>(flags));
}

auto Submission::read(std::int_fast32_t fileDescriptor, void *buffer, std::uint_fast32_t bufferLength,
                      std::uint_fast64_t offset) noexcept -> void {
    io_uring_prep_read(this->submission, static_cast<int>(fileDescriptor), buffer, bufferLength, offset);
}

auto Submission::receive(std::int_fast32_t fileDescriptor, void *buffer, std::uint_fast64_t bufferLength,
                         std::int_fast32_t flags) noexcept -> void {
    io_uring_prep_recv_multishot(this->submission, static_cast<int>(fileDescriptor), buffer, bufferLength,
                                 static_cast<int>(flags));
}

auto Submission::send(std::int_fast32_t fileDescriptor, const void *buffer, std::uint_fast64_t bufferLength,
                      std::int_fast32_t flags, std::uint_fast32_t zeroCopyFlags) noexcept -> void {
    io_uring_prep_send_zc(this->submission, static_cast<int>(fileDescriptor), buffer, bufferLength,
                          static_cast<int>(flags), zeroCopyFlags);
}

auto Submission::cancel(std::int_fast32_t fileDescriptor, std::uint_fast32_t flags) noexcept -> void {
    io_uring_prep_cancel_fd(this->submission, static_cast<int>(fileDescriptor), flags);
}

auto Submission::close(std::uint_fast32_t fileDescriptorIndex) noexcept -> void {
    io_uring_prep_close_direct(this->submission, fileDescriptorIndex);
}
