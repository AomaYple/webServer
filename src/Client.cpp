#include "Client.h"

#include <sys/epoll.h>

#include <cstring>

#include "Log.h"

using std::string, std::string_view, std::source_location;

Client::Client(int socket, string &&information, unsigned short timeout)
    : socket{socket}, event{EPOLLIN}, timeout{timeout}, keepAlive{true}, information{std::move(information)} {}

Client::Client(Client &&other) noexcept
    : socket{other.socket}, event{other.event}, timeout{other.timeout}, keepAlive{other.keepAlive},
      information{std::move(other.information)}, receiveBuffer{std::move(other.receiveBuffer)},
      sendBuffer{std::move(other.sendBuffer)} {
    other.socket = -1;
}

auto Client::operator=(Client &&other) noexcept -> Client & {
    if (this != &other) {
        this->socket = other.socket;
        this->event = other.event;
        this->timeout = other.timeout;
        this->keepAlive = other.keepAlive;
        this->information = std::move(other.information);
        this->receiveBuffer = std::move(other.receiveBuffer);
        this->sendBuffer = std::move(other.sendBuffer);
        other.socket = -1;
    }
    return *this;
}

auto Client::receive(source_location sourceLocation) -> void {
    this->event = EPOLLOUT;

    ssize_t allReceivedBytes{0};

    while (true) {
        auto data{this->receiveBuffer.writableData()};

        ssize_t receivedBytes{::read(this->socket, data.first, data.second)};

        if (receivedBytes > 0) {
            allReceivedBytes += receivedBytes;

            this->receiveBuffer.adjustWrite(receivedBytes);
        } else if (receivedBytes == 0) {
            this->event = 0;

            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                this->event = 0;

                Log::add(sourceLocation, Level::WARN, this->information + " receive error: " + std::strerror(errno));
            }

            break;
        }
    }
}

auto Client::send(source_location sourceLocation) -> void {
    this->event = this->keepAlive ? EPOLLIN : 0;

    string_view data{this->sendBuffer.read()};

    ssize_t allSentBytes{0};

    while (allSentBytes < data.size()) {
        ssize_t sentBytes{::write(this->socket, data.data() + allSentBytes, data.size() - allSentBytes)};

        if (sentBytes > 0) allSentBytes += sentBytes;
        else if (sentBytes == 0) {
            this->event = 0;

            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                this->event = EPOLLOUT;

                Log::add(sourceLocation, Level::INFO, " too much data can not be sent to " + this->information);
            } else {
                this->event = 0;

                Log::add(sourceLocation, Level::WARN, this->information + " send error: " + std::strerror(errno));
            }

            break;
        }
    }

    if (allSentBytes > 0) this->sendBuffer.adjustRead(allSentBytes);
}

auto Client::read() -> string {
    string data{this->receiveBuffer.read()};

    this->receiveBuffer.adjustRead(data.size());

    return data;
}

auto Client::write(string_view data) -> void { this->sendBuffer.write(data); }

auto Client::get() const -> int { return this->socket; }

auto Client::getEvent() const -> unsigned int { return this->event; }

auto Client::getTimeout() const -> unsigned short { return this->timeout; }

auto Client::getInformation() const -> string_view { return this->information; }

auto Client::setKeepAlive(bool value) -> void { this->keepAlive = value; }

Client::~Client() {
    if (this->socket != -1) {
        if (close(this->socket) == -1)
            Log::add(source_location::current(), Level::ERROR,
                     this->information + " close error: " + std::strerror(errno));
        else
            Log::add(source_location::current(), Level::INFO, this->information + " is closed");
    }
}