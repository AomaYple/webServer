#include "Client.h"

#include "Buffer.h"
#include "Event.h"
#include "Submission.h"

using std::string, std::shared_ptr;

Client::Client(int socket, shared_ptr<Ring> &ring, Buffer &buffer, unsigned short timeout)
        : self{socket}, timeout{timeout}, keepAlive{true}, ring{ring} {
    this->time();

    this->receive(buffer);
}

Client::Client(Client &&client) noexcept
        : self{client.self},
          timeout{client.timeout},
          keepAlive{client.keepAlive},
          unSendData{std::move(client.unSendData)},
          ring{std::move(client.ring)} {
    client.self = -1;
}

auto Client::operator=(Client &&client) noexcept -> Client & {
    if (this != &client) {
        this->self = client.self;
        this->timeout = client.timeout;
        this->keepAlive = client.keepAlive;
        this->unSendData = std::move(client.unSendData);
        this->ring = std::move(client.ring);
        client.self = -1;
    }
    return *this;
}

auto Client::time() -> void {
    Submission submission{this->ring};

    Event event{Type::TIME, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));

    __kernel_timespec timespec{this->timeout, 0};
    submission.time(&timespec, 0, 0);
}

auto Client::getKeepAlive() const -> bool { return this->keepAlive; }

auto Client::setKeepAlive(bool option) -> void { this->keepAlive = option; }

auto Client::updateTime() -> void {
    Submission submission{this->ring};

    Event event{Type::RENEW, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));

    submission.setFlags(IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);

    __kernel_timespec timespec{this->timeout, 0};
    submission.updateTime(&timespec, reinterpret_cast<unsigned long long>(this), 0);
}

auto Client::receive(Buffer &buffer) -> void {
    Submission submission{this->ring};

    Event event{Type::RECEIVE, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);
    submission.setBufferId(buffer.getId());

    submission.receive(this->self, nullptr, 0, IORING_RECVSEND_POLL_FIRST);
}

auto Client::send(string &&data) -> void {
    this->unSendData = std::move(data);

    Submission submission{this->ring};

    Event event{Type::SEND, this->self};
    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_FIXED_FILE);

    submission.send(this->self, this->unSendData.data(), this->unSendData.size(), 0, 0);
}

auto Client::write(string &&data) -> void { this->receivedData += data; }

auto Client::read() -> string {
    string returnData{std::move(this->receivedData)};
    this->receivedData = {};
    return returnData;
}

auto Client::cancel() -> void {
    Submission submission{this->ring};

    Event event{Type::CANCEL, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);

    submission.cancel(this->self, IORING_ASYNC_CANCEL_ALL);
}

auto Client::close() -> void {
    Submission submission{this->ring};

    Event event{Type::CLOSE, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);

    submission.close(this->self);
}

Client::~Client() {
    this->cancel();

    this->close();
}
