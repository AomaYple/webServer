#include "Client.hpp"

#include <liburing/io_uring.h>

Client::Client(int fileDescriptor, unsigned long seconds) noexcept : FileDescriptor{fileDescriptor}, seconds{seconds} {}

auto Client::getSeconds() const noexcept -> unsigned long { return this->seconds; }

auto Client::receive(int ringBufferId) const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(), Submission::Receive{std::span<std::byte>{}, 0, ringBufferId},
        IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT, 0
    });

    return awaiter;
}

auto Client::send(std::span<const std::byte> data) const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(), Submission::Send{data, 0, 0},
         IOSQE_FIXED_FILE, 0
    });

    return awaiter;
}
