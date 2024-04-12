#include "Client.hpp"

Client::Client(int fileDescriptor, RingBuffer &&ringBuffer, unsigned long seconds) noexcept :
    FileDescriptor{fileDescriptor}, ringBuffer{std::move(ringBuffer)}, seconds{seconds} {}

auto Client::getSeconds() const noexcept -> unsigned long { return this->seconds; }

auto Client::receive() const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(), Submission::Receive{{}, 0, this->ringBuffer.getId()},
        IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT, 0
    });

    return awaiter;
}

auto Client::getReceivedData(unsigned short index, unsigned int size) -> std::vector<std::byte> {
    return this->ringBuffer.readFromBuffer(index, size);
}

auto Client::writeToBuffer(std::span<const std::byte> data) -> void {
    this->buffer.insert(this->buffer.cend(), data.cbegin(), data.cend());
}

auto Client::readFromBuffer() const noexcept -> std::span<const std::byte> { return this->buffer; }

auto Client::clearBuffer() noexcept -> void { this->buffer.clear(); }

auto Client::send() const noexcept -> Awaiter {
    Awaiter awaiter;
    awaiter.setSubmission(Submission{
        this->getFileDescriptor(), Submission::Send{this->buffer, 0, 0},
         IOSQE_FIXED_FILE, 0
    });

    return awaiter;
}
