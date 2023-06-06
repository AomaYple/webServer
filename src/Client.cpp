#include "Client.h"

#include "Buffer.h"
#include "Submission.h"

Client::Client(int socket, Ring &ring, Buffer &buffer)
    : self{socket}, event{Event::RECEIVE}, timeout{60}, keepAlive{true} {
    this->time(ring);

    this->receive(ring, buffer);
}

Client::Client(Client &&client) noexcept
    : self{client.self},
      event{client.event},
      timeout{client.timeout},
      keepAlive{client.keepAlive},
      sendData{std::move(client.sendData)} {
    client.self = -1;
}

auto Client::operator=(Client &&client) noexcept -> Client & {
    if (this != &client) {
        this->self = client.self;
        this->event = client.event;
        this->timeout = client.timeout;
        this->keepAlive = client.keepAlive;
        this->sendData = std::move(client.sendData);
        client.self = -1;
    }
    return *this;
}

auto Client::getEvent() const -> Event { return this->event; }

auto Client::setEvent(Event newEvent) -> void { this->event = newEvent; }

auto Client::getKeepAlive() const -> bool { return this->keepAlive; }

auto Client::setKeepAlive(bool option) -> void { this->keepAlive = option; }

auto Client::updateTime(Ring &ring) -> void {
    Submission submission{ring};

    submission.setData(this);
    submission.setFlags(IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);

    __kernel_timespec timespec{this->timeout, 0};
    submission.updateTime(&timespec, reinterpret_cast<unsigned long long>(this), 0);
}

auto Client::receive(Ring &ring, Buffer &buffer) -> void {
    this->updateTime(ring);

    this->event = Event::RECEIVE;

    Submission submission{ring};

    submission.setData(this);
    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_BUFFER_SELECT);
    submission.setBufferId(buffer.getId());

    submission.receive(this->self, nullptr, 0, IORING_RECVSEND_POLL_FIRST);
}

auto Client::send(std::string &&data, Ring &ring) -> void {
    this->sendData = std::move(data);

    this->event = Event::SEND;

    Submission submission{ring};

    submission.setData(this);
    submission.setFlags(IOSQE_FIXED_FILE);

    submission.send(this->self, this->sendData.data(), this->sendData.size(), 0, 0);
}

auto Client::close(Ring &ring) -> void {
    this->cancel(ring);

    this->event = Event::CLOSE;

    Submission submission{ring};

    submission.setData(this);

    submission.close(this->self);
}

auto Client::time(Ring &ring) -> void {
    Submission submission{ring};

    submission.setData(this);

    __kernel_timespec timespec{this->timeout, 0};
    submission.time(&timespec, 0, 0);
}

auto Client::cancel(Ring &ring) -> void {
    Submission submission{ring};

    submission.setData(this);
    submission.setFlags(IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);

    submission.cancel(this, IORING_ASYNC_CANCEL_ALL);
}
