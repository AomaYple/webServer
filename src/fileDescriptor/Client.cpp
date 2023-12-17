#include "Client.hpp"

#include "../ring/Submission.hpp"

Client::Client(int fileDescriptor, std::shared_ptr<Ring> ring, RingBuffer &&ringBuffer,
               std::chrono::seconds seconds) noexcept
    : FileDescriptor{fileDescriptor, std::move(ring)}, ringBuffer{std::move(ringBuffer)}, seconds{seconds} {}

auto Client::getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte> {
    return this->ringBuffer.getData(index, dataSize);
}

auto Client::getSeconds() const noexcept -> std::chrono::seconds { return this->seconds; }

auto Client::getBuffer() noexcept -> std::vector<std::byte> & { return this->buffer; }

auto Client::receive() const -> void {
    const Submission submission{Event{Event::Type::receive, this->getFileDescriptor()},
                                IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT,
                                Submission::ReceiveParameters{0, this->ringBuffer.getId()}};

    this->getRing()->submit(submission);
}

auto Client::send() const -> void {
    const Submission submission{Event{Event::Type::send, this->getFileDescriptor()}, IOSQE_FIXED_FILE,
                                Submission::SendParameters{this->buffer, 0, 0}};

    this->getRing()->submit(submission);
}
