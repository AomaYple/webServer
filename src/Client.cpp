#include "Client.h"

#include "Log.h"
#include "Submission.h"
#include "UserData.h"

using std::runtime_error;
using std::shared_ptr;
using std::source_location;
using std::string;

Client::Client(int fileDescriptor, unsigned short timeout, const shared_ptr<UserRing> &userRing) noexcept
    : fileDescriptor{fileDescriptor}, timeout{timeout}, userRing{userRing} {}

Client::Client(Client &&other) noexcept
    : fileDescriptor{other.fileDescriptor}, timeout{other.timeout}, receivedData{std::move(other.receivedData)},
      unSendData{std::move(other.unSendData)}, userRing{std::move(other.userRing)} {
    other.fileDescriptor = -1;
}

auto Client::operator=(Client &&other) noexcept -> Client & {
    if (this != &other) {
        this->fileDescriptor = other.fileDescriptor;
        this->timeout = other.timeout;
        this->receivedData = std::move(other.receivedData);
        this->unSendData = std::move(other.unSendData);
        this->userRing = std::move(other.userRing);
        other.fileDescriptor = -1;
    }
    return *this;
}

auto Client::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto Client::getTimeout() const noexcept -> unsigned short { return this->timeout; }

auto Client::receive(unsigned short bufferRingId) -> void {
    Submission submission{this->userRing->getSubmission()};

    UserData userData{Type::RECEIVE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.receive(this->fileDescriptor, nullptr, 0, 0);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferGroup(bufferRingId);
}

auto Client::writeReceivedData(string &&data) -> void { this->receivedData += data; }

auto Client::readReceivedData() noexcept -> string {
    string data{std::move(this->receivedData)};

    this->receivedData.clear();

    return data;
}

auto Client::send(string &&data) -> void {
    this->unSendData = std::move(data);

    Submission submission{this->userRing->getSubmission()};

    UserData userData{Type::SEND, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.send(this->fileDescriptor, this->unSendData.data(), this->unSendData.size(), 0, 0);

    submission.setFlags(IOSQE_FIXED_FILE);
}

Client::~Client() {
    if (this->fileDescriptor != -1) {
        try {
            this->cancel();

            this->close();
        } catch (const runtime_error &runtimeError) {
            Log::produce(source_location::current(), Level::ERROR, runtimeError.what());
        }
    }
}

auto Client::cancel() -> void {
    Submission submission{this->userRing->getSubmission()};

    UserData userData{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.cancel(this->fileDescriptor, IORING_ASYNC_CANCEL_ALL);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Client::close() -> void {
    Submission submission{this->userRing->getSubmission()};

    UserData userData{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.close(this->fileDescriptor);

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
