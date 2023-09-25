#include "Client.hpp"

#include "../userRing/Submission.hpp"
#include "../userRing/UserData.hpp"

Client::Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, timeout{timeout} {}

auto Client::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Client::getTimeout() const noexcept -> unsigned char { return this->timeout; }

auto Client::startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void {
    constexpr unsigned int flags{0};
    const Submission submission{sqe, this->fileDescriptorIndex, {}, flags};

    const UserData userData{EventType::Receive, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

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

auto Client::writeData(std::span<const std::byte> data) -> void {
    this->buffer.insert(this->buffer.cend(), data.cbegin(), data.cend());
}

auto Client::send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter & {
    this->buffer = std::move(data);

    const Submission submission{sqe, this->fileDescriptorIndex, this->buffer, 0, 0};

    const UserData userData{EventType::Send, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::setSendGenerator(Generator &&generator) noexcept -> void { this->sendGenerator = std::move(generator); }

auto Client::resumeSend(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->sendGenerator.resume();
}

auto Client::readData() noexcept -> std::span<const std::byte> { return this->buffer; }

auto Client::clearBuffer() noexcept -> void { this->buffer.clear(); }

auto Client::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{EventType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

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

    const UserData userData{EventType::Close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    return this->awaiter;
}

auto Client::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Client::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeGenerator.resume();
}
