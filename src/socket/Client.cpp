#include "Client.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"

using namespace std;

Client::Client(unsigned int fileDescriptorIndex, unsigned char timeout, const shared_ptr<UserRing> &userRing) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, timeout{timeout}, userRing{userRing} {}

Client::Client(Client &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, timeout{other.timeout},
      receivedData{std::move(other.receivedData)}, unSendData{std::move(other.unSendData)},
      userRing{std::move(other.userRing)} {}

auto Client::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Client::getTimeout() const noexcept -> unsigned char { return this->timeout; }

auto Client::receive(unsigned short bufferRingId) const -> void {
    unsigned int flags{0};
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, {}, flags};

    const UserData userData{Type::RECEIVE, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferRingId(bufferRingId);
}

auto Client::writeReceivedData(span<const byte> data) -> void {
    this->receivedData.insert(this->receivedData.end(), data.begin(), data.end());
}

auto Client::readReceivedData() noexcept -> vector<byte> {
    vector<byte> data{std::move(this->receivedData)};

    this->receivedData.clear();

    return data;
}
auto Client::send(vector<byte> &&data) -> void {
    this->unSendData = std::move(data);

    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, this->unSendData, 0, 0};

    const UserData userData{Type::SEND, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

Client::~Client() {
    if (this->userRing != nullptr) {
        try {
            this->cancel();

            this->close();
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}

auto Client::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::CANCEL, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Client::close() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex};

    const UserData userData{Type::CLOSE, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
