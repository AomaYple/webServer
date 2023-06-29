#include "Client.h"

#include "Event.h"
#include "Http.h"
#include "Submission.h"

using std::string_view, std::shared_ptr;

Client::Client(int socket, unsigned short timeout, const shared_ptr<Ring> &ring)
    : socket{socket}, timeout{timeout}, ring{ring} {}

Client::Client(Client &&other) noexcept
    : socket{other.socket}, timeout{other.timeout}, receiveBuffer{std::move(other.receiveBuffer)},
      sendBuffer{std::move(other.sendBuffer)}, ring{std::move(other.ring)} {
    other.socket = -1;
}

auto Client::operator=(Client &&other) noexcept -> Client & {
    if (this != &other) {
        this->socket = other.socket;
        this->timeout = other.timeout;
        this->receiveBuffer = std::move(other.receiveBuffer);
        this->sendBuffer = std::move(other.sendBuffer);
        this->ring = std::move(other.ring);
        other.socket = -1;
    }
    return *this;
}

auto Client::receive() -> void {
    Submission submission{this->ring->getSubmission()};

    Event event{Type::RECEIVE, this->socket};
    submission.setData(reinterpret_cast<unsigned long long &>(event));
}

auto Client::get() const -> int { return this->socket; }

auto Client::getTimeout() const -> unsigned short { return this->timeout; }

Client::~Client() {
    if (this->socket != -1) {
        Submission submission{this->ring->getSubmission()};

        Event event{Type::CLOSE, this->socket};
        submission.setData(reinterpret_cast<unsigned long long &>(event));
        submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);

        submission.close(this->socket);
    }
}
