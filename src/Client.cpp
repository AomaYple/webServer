#include "Client.h"

#include "BufferRing.h"
#include "Event.h"
#include "Submission.h"

using std::string, std::shared_ptr;

Client::Client(int fileDescriptor, const shared_ptr<Ring> &ring, BufferRing &bufferRing, unsigned short timeout)
    : self{fileDescriptor}, timeoutTime{timeout}, keepAlive{true}, ring{ring} {
    this->timeout();

    this->receive(bufferRing);
}

Client::Client(Client &&client) noexcept
    : self{client.self},
      timeoutTime{client.timeoutTime},
      keepAlive{client.keepAlive},
      receivedData{std::move(client.receivedData)},
      ring{std::move(client.ring)} {
    client.self = -1;
}

auto Client::operator=(Client &&client) noexcept -> Client & {
    if (this != &client) {
        this->self = client.self;
        this->timeoutTime = client.timeoutTime;
        this->keepAlive = client.keepAlive;
        this->receivedData = std::move(client.receivedData), this->ring = std::move(client.ring);
        client.self = -1;
    }
    return *this;
}

auto Client::receive(BufferRing &bufferRing) -> void {
    Submission submission{*this->ring};

    Event event{Type::RECEIVE, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);
    submission.setBufferGroup(bufferRing.getId());

    submission.receive(this->self, nullptr, 0, 0);
}

auto Client::write(string &&data) -> void { this->receivedData += data; }

auto Client::read() -> string {
    string data{std::move(this->receivedData)};

    this->receivedData = {};

    return data;
}

auto Client::updateTimeout() -> void {
    Submission submission{*this->ring};

    Event event{Type::UPDATE_TIMEOUT, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);

    Event data{Type::TIMEOUT, this->self};
    __kernel_timespec timespec{this->timeoutTime, 0};
    submission.updateTimeout(&timespec, reinterpret_cast<unsigned long long &>(data), IORING_TIMEOUT_ETIME_SUCCESS);
}

Client::~Client() {
    if (this->self != -1) {
        this->removeTimeout();

        this->cancel();

        this->close();
    }
}

auto Client::timeout() -> void {
    Submission submission{*this->ring};

    Event event{Type::TIMEOUT, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));

    __kernel_timespec timespec{this->timeoutTime, 0};
    submission.timeout(&timespec, 0, IORING_TIMEOUT_ETIME_SUCCESS);
}

auto Client::removeTimeout() -> void {
    Submission submission{*this->ring};

    Event event{Type::REMOVE_TIMEOUT, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_IO_HARDLINK | IOSQE_CQE_SKIP_SUCCESS);

    Event data{Type::TIMEOUT, this->self};
    submission.removeTimeout(reinterpret_cast<unsigned long long &>(data), 0);
}

auto Client::cancel() -> void {
    Submission submission{*this->ring};

    Event event{Type::CANCEL_FILE_DESCRIPTOR, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_HARDLINK | IOSQE_CQE_SKIP_SUCCESS);

    submission.cancelFileDescriptor(this->self, IORING_ASYNC_CANCEL_ALL);
}

auto Client::close() -> void {
    Submission submission{*this->ring};

    Event event{Type::CLOSE, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);

    submission.close(this->self);
}
