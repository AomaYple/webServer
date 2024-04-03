#include "Client.hpp"

#include "../ring/Submission.hpp"

Client::Client(int fileDescriptor, std::shared_ptr<Ring> ring, unsigned long seconds)
    : FileDescriptor{fileDescriptor, std::move(ring)}, ringBuffer{1, 1024, fileDescriptor, this->getRing()},
      seconds{seconds} {}

auto Client::getSeconds() const noexcept -> unsigned long { return this->seconds; }

auto Client::setFirstGenerator(Generator &&generator) noexcept -> void {
    this->generators.first = std::move(generator);
}

auto Client::resumeFirstGenerator(Outcome outcome) -> void {
    this->setAwaiterOutcome(outcome);
    this->generators.first.resume();
}

auto Client::setSecondGenerator(Generator &&generator) noexcept -> void {
    this->generators.second = std::move(generator);
}

auto Client::resumeSecondGenerator(Outcome outcome) -> void {
    this->setAwaiterOutcome(outcome);
    this->generators.second.resume();
}

auto Client::startReceive() const -> void {
    const Submission submission{Event{Event::Type::receive, this->getFileDescriptor()},
                                IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT,
                                Submission::Receive{0, this->ringBuffer.getId()}};
    this->getRing()->submit(submission);
}

auto Client::receive() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::getReceivedData(unsigned short index, unsigned int size) -> std::vector<std::byte> {
    return this->ringBuffer.readFromBuffer(index, size);
}

auto Client::writeToBuffer(std::span<const std::byte> data) -> void {
    this->buffer.insert(this->buffer.cend(), data.cbegin(), data.cend());
}

auto Client::readFromBuffer() const noexcept -> std::span<const std::byte> { return this->buffer; }

auto Client::clearBuffer() noexcept -> void { this->buffer.clear(); }

auto Client::send() const -> const Awaiter & {
    const Submission submission{Event{Event::Type::send, this->getFileDescriptor()}, IOSQE_FIXED_FILE,
                                Submission::Send{this->buffer, 0, 0}};
    this->getRing()->submit(submission);

    return this->awaiter;
}

auto Client::setAwaiterOutcome(Outcome outcome) noexcept -> void { this->awaiter.setOutcome(outcome); }
