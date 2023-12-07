#include "Client.hpp"

#include "../userRing/Submission.hpp"

Client::Client(int fileDescriptorIndex, BufferRing &&bufferRing, unsigned long seconds) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, bufferRing{std::move(bufferRing)}, seconds{seconds}, awaiter{} {}

auto Client::getFileDescriptorIndex() const noexcept -> int { return this->fileDescriptorIndex; }

auto Client::getBufferRingData(unsigned short bufferIndex, unsigned int dataSize) noexcept -> std::vector<std::byte> {
    return this->bufferRing.getData(bufferIndex, dataSize);
}

auto Client::getSeconds() const noexcept -> unsigned long { return this->seconds; }

auto Client::writeToBuffer(std::span<const std::byte> data) noexcept -> void {
    this->buffer.insert(this->buffer.cend(), data.cbegin(), data.cend());
}

auto Client::readFromBuffer() const noexcept -> std::span<const std::byte> { return this->buffer; }

auto Client::clearBuffer() noexcept -> void { this->buffer.clear(); }

auto Client::setAwaiterOutcome(Outcome outcome) noexcept -> void { this->awaiter.setOutcome(outcome); }

auto Client::setReceiveGenerator(Generator &&generator) noexcept -> void {
    this->receiveGenerator = std::move(generator);
}

auto Client::getReceiveSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::receive, this->fileDescriptorIndex}, IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT,
                      Submission::ReceiveParameters{{}, this->bufferRing.getId()}};
}

auto Client::receive() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::resumeReceive() const -> void { this->receiveGenerator.resume(); }

auto Client::setSendGenerator(Generator &&generator) noexcept -> void { this->sendGenerator = std::move(generator); }

auto Client::getSendSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::send, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
                      Submission::SendParameters{this->buffer, 0, 0}};
}

auto Client::send() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::resumeSend() const -> void { this->sendGenerator.resume(); }

auto Client::setCancelGenerator(Generator &&generator) noexcept -> void {
    this->cancelGenerator = std::move(generator);
}

auto Client::getCancelSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::cancel, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
                      Submission::CancelParameters{IORING_ASYNC_CANCEL_ALL}};
}

auto Client::cancel() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::resumeCancel() const -> void { this->cancelGenerator.resume(); }

auto Client::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Client::getCloseSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::close, this->fileDescriptorIndex}, 0, Submission::CloseParameters{}};
}

auto Client::close() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::resumeClose() const -> void { this->closeGenerator.resume(); }
