#include "Client.hpp"

#include "../userRing/Submission.hpp"
#include "../userRing/UserData.hpp"

Client::Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, timeout{timeout}, receiveTask{nullptr}, sendTask{nullptr},
      cancelTask{nullptr}, closeTask{nullptr} {}

Client::Client(Client &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, timeout{other.timeout}, buffer{std::move(other.buffer)},
      receiveTask{std::move(other.receiveTask)}, sendTask{std::move(other.sendTask)},
      cancelTask{std::move(other.cancelTask)}, closeTask{std::move(other.closeTask)},
      awaiter{std::move(other.awaiter)} {}

auto Client::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Client::getTimeout() const noexcept -> unsigned char { return this->timeout; }

auto Client::startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), {}, 0};

    const UserData userData{TaskType::Receive, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferRingId(bufferRingId);
}

auto Client::receive() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::setReceiveTask(Task &&task) noexcept -> void { this->receiveTask = std::move(task); }

auto Client::resumeReceive(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->receiveTask.resume();
}

auto Client::writeData(std::span<const std::byte> data) -> void {
    this->buffer.insert(this->buffer.end(), data.begin(), data.end());
}

auto Client::send(io_uring_sqe *sqe, std::vector<std::byte> &&data) noexcept -> const Awaiter & {
    this->buffer = std::move(data);

    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), this->buffer, 0, 0};

    const UserData userData{TaskType::Send, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::setSendTask(Task &&task) noexcept -> void { this->sendTask = std::move(task); }

auto Client::resumeSend(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->sendTask.resume();
}

auto Client::readData() noexcept -> std::span<const std::byte> { return this->buffer; }

auto Client::clearBuffer() noexcept -> void { this->buffer.clear(); }

auto Client::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), IORING_ASYNC_CANCEL_ALL};

    const UserData userData{TaskType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::setCancelTask(Task &&task) noexcept -> void { this->cancelTask = std::move(task); }

auto Client::resumeCancel(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelTask.resume();
}

auto Client::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{TaskType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}

auto Client::setCloseTask(Task &&task) noexcept -> void { this->closeTask = std::move(task); }

auto Client::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeTask.resume();
}
