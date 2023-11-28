#include "Client.hpp"

#include "../userRing/Event.hpp"
#include "../userRing/Submission.hpp"

Client::Client(unsigned int fileDescriptorIndex, unsigned short timeout) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, timeout{timeout} {}

auto Client::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Client::getTimeout() const noexcept -> unsigned short { return this->timeout; }

auto Client::getBuffer() noexcept -> std::vector<std::byte> & { return this->buffer; }

auto Client::startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void {
    constexpr unsigned int flags{0};
    const Submission submission{sqe, this->fileDescriptorIndex, {}, flags};

    const Event event{Event::Type::receive, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferRingId(bufferRingId);
}

auto Client::receive() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::setReceiveGenerator(Generator &&generator) noexcept -> void {
    this->receiveGenerator = std::move(generator);
}

auto Client::resumeReceive(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->receiveGenerator.resume();
}

auto Client::send(io_uring_sqe *sqe) noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, this->buffer, 0, 0};

    const Event event{Event::Type::send, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::setSendGenerator(Generator &&generator) noexcept -> void { this->sendGenerator = std::move(generator); }

auto Client::resumeSend(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->sendGenerator.resume();
}

auto Client::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const Event event{Event::Type::cancel, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::setCancelGenerator(Generator &&generator) noexcept -> void {
    this->cancelGenerator = std::move(generator);
}

auto Client::resumeCancel(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelGenerator.resume();
}

auto Client::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const Event event{Event::Type::close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    return this->awaiter;
}

auto Client::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Client::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeGenerator.resume();
}
