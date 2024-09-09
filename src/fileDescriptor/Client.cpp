#include "Client.hpp"

#include <linux/io_uring.h>

Client::Client(const int fileDescriptor, const std::chrono::seconds seconds) noexcept :
    FileDescriptor{fileDescriptor}, seconds{seconds} {}

auto Client::getSeconds() const noexcept -> std::chrono::seconds { return this->seconds; }

auto Client::receive(const int ringBufferId) const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(),
        IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT,
        0,
        0,
        Submission::Receive{std::span<std::byte>{}, 0, ringBufferId},
    });

    return awaiter;
}

auto Client::send(const std::span<const std::byte> data) const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(),
        IOSQE_FIXED_FILE,
        0,
        0,
        Submission::Send{data, 0, 0},
    });

    return awaiter;
}
