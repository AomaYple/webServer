#include "Client.hpp"

#include "../ring/Submission.hpp"

Client::Client(int fileDescriptor, std::shared_ptr<Ring> ring, unsigned long seconds)
    : FileDescriptor{fileDescriptor, std::move(ring)}, ringBuffer{1, 1024, fileDescriptor, this->getRing()},
      seconds{seconds} {}

auto Client::getSeconds() const noexcept -> unsigned long { return this->seconds; }

auto Client::receive() const -> void {
    const Submission submission{Event{Event::Type::receive, this->getFileDescriptor()},
                                IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT,
                                Submission::ReceiveParameters{0, this->ringBuffer.getId()}};
    this->getRing()->submit(submission);
}

auto Client::getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte> {
    return this->ringBuffer.getData(index, dataSize);
}

auto Client::getBuffer() noexcept -> std::vector<std::byte> & { return this->buffer; }

auto Client::send() -> void {
    const Submission submission{Event{Event::Type::send, this->getFileDescriptor()}, IOSQE_FIXED_FILE,
                                Submission::SendParameters{this->buffer, 0, 0}};
    this->getRing()->submit(submission);
}
