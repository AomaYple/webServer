#include "Client.h"

#include "../base/Submission.h"
#include "../base/UserData.h"

using namespace std;

Client::Client(unsigned int fileDescriptorIndex, unsigned char timeout) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, timeout{timeout} {}

Client::Client(Client &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, timeout{other.timeout},
      receivedData{std::move(other.receivedData)}, unSendData{std::move(other.unSendData)},
      awaiter{std::move(other.awaiter)} {}

auto Client::getTimeout() const noexcept -> unsigned char { return this->timeout; }

auto Client::setResult(pair<int, unsigned int> result) noexcept -> void { this->awaiter.setResult(result); }

auto Client::startReceive(io_uring_sqe *sqe, unsigned short bufferRingId) const noexcept -> void {
    unsigned int flags{0};
    const Submission submission{sqe, this->fileDescriptorIndex, {}, flags};

    const UserData userData{EventType::Receive, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferRingId(bufferRingId);
}

auto Client::receive() const noexcept -> const Awaiter & { return this->awaiter; }

auto Client::writeReceivedData(span<const byte> data) noexcept -> void {
    this->receivedData.insert(this->receivedData.end(), data.begin(), data.end());
}

auto Client::readReceivedData() noexcept -> vector<byte> {
    vector<byte> data{std::move(this->receivedData)};

    this->receivedData.clear();

    return data;
}

auto Client::send(io_uring_sqe *sqe, vector<byte> &&data) noexcept -> const Awaiter & {
    this->unSendData = std::move(data);

    const Submission submission{sqe, this->fileDescriptorIndex, this->unSendData, 0, 0};

    const UserData userData{EventType::Send, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{EventType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Client::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{EventType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}
