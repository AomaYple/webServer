#include "Client.h"

#include "Event.h"
#include "Log.h"
#include "Submission.h"

using std::string, std::shared_ptr, std::runtime_error, std::source_location;

Client::Client(int socket, const shared_ptr<UserRing> &userRing) noexcept : socket{socket}, userRing{userRing} {}

Client::Client(Client &&other) noexcept
    : socket{other.socket}, receivedData{std::move(other.receivedData)}, unSendData{std::move(other.unSendData)},
      userRing{std::move(other.userRing)} {
    other.socket = -1;
}

auto Client::operator=(Client &&other) noexcept -> Client & {
    if (this != &other) {
        this->socket = other.socket;
        this->receivedData = std::move(other.receivedData);
        this->unSendData = std::move(other.unSendData);
        this->userRing = std::move(other.userRing);
        other.socket = -1;
    }
    return *this;
}

auto Client::receive(unsigned short bufferRingId) -> void {
    Submission submission{this->userRing->getSubmission()};

    Event event{Type::RECEIVE, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.receive(this->socket, nullptr, 0, 0);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);

    submission.setBufferGroup(bufferRingId);
}

auto Client::writeReceivedData(string &&data) noexcept -> void { this->receivedData += data; }

auto Client::readReceivedData() noexcept -> string {
    string data{std::move(this->receivedData)};

    this->receivedData.clear();

    return data;
}

auto Client::send(string &&data) -> void {
    this->unSendData = std::move(data);

    Submission submission{this->userRing->getSubmission()};

    Event event{Type::SEND, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.send(this->socket, this->unSendData.data(), this->unSendData.size(), 0, 0);

    submission.setFlags(IOSQE_FIXED_FILE);
}

Client::~Client() {
    if (this->socket != -1) {
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

    Event event{Type::CANCEL, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.cancel(this->socket, IORING_ASYNC_CANCEL_ALL);

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Client::close() -> void {
    Submission submission{this->userRing->getSubmission()};

    Event event{Type::CLOSE, this->socket};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.close(this->socket);

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
