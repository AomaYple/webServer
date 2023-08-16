#include "Client.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"

using std::byte, std::queue, std::shared_ptr, std::vector;

Client::Client(int fileDescriptor, std::uint_least8_t timeout, const shared_ptr<UserRing> &userRing) noexcept
    : fileDescriptor{fileDescriptor}, timeout{timeout}, userRing{userRing} {}

Client::Client(Client &&other) noexcept
    : fileDescriptor{other.fileDescriptor}, timeout{other.timeout}, receivedData{std::move(other.receivedData)},
      unSendData{std::move(other.unSendData)}, userRing{std::move(other.userRing)} {
    other.fileDescriptor = -1;
}

auto Client::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto Client::getTimeout() const noexcept -> std::uint_least8_t { return this->timeout; }

auto Client::receive(__u16 bufferRingId) const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptor, {}, 0};

    const UserData userData{Type::RECEIVE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferRingId(bufferRingId);
}

auto Client::writeReceivedData(vector<byte> &&data) -> void {
    this->receivedData.insert(this->receivedData.end(), data.begin(), data.end());
}

auto Client::readReceivedData() noexcept -> vector<byte> {
    vector<byte> data{std::move(this->receivedData)};

    this->receivedData.clear();

    return data;
}

auto Client::writeUnSendData(queue<vector<byte>> &&data) noexcept -> void { this->unSendData = std::move(data); }

auto Client::send() -> void {
    if (!this->unSendData.empty()) {
        this->unSendData.pop();

        int flags{0};
        const Submission submission{this->userRing->getSqe(), this->fileDescriptor, this->unSendData.front(), flags, 0};

        const UserData userData{Type::SEND, this->fileDescriptor};
        submission.setUserData(reinterpret_cast<const __u64 &>(userData));

        submission.setFlags(IOSQE_FIXED_FILE);
    }
}

Client::~Client() {
    if (this->fileDescriptor != -1) {
        try {
            this->cancel();

            this->close();
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}

auto Client::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptor, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Client::close() const -> void {
    const Submission submission{this->userRing->getSqe(), static_cast<unsigned int>(this->fileDescriptor)};

    const UserData userData{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
