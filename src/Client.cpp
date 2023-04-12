#include "Client.h"
#include "Log.h"

#include <cstring>

#include <sys/socket.h>
#include <sys/epoll.h>

using std::string, std::string_view, std::source_location;

Client::Client(int fileDescriptor, std::string information, unsigned short timeout, const std::source_location &sourceLocation) :
        self(fileDescriptor), event(EPOLLIN), timeout(timeout), keepAlive(false), information(std::move(information)) {
    linger linger {1, 5};

    if (setsockopt(this->self, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
        Log::add(sourceLocation, Level::ERROR, this->information + " location linger error: " + strerror(errno));
}

Client::Client(Client &&client) noexcept : self(client.self), event(client.event), timeout(client.timeout), keepAlive(client.keepAlive),
        information(std::move(client.information)), sendBuffer(std::move(client.sendBuffer)), receiveBuffer(std::move(client.receiveBuffer)) {
    client.self = -1;
    client.event = 0;
    client.timeout = 0;
    client.keepAlive = false;
}

auto Client::operator=(Client &&client) noexcept -> Client & {
    if (this != &client) {
        this->self = client.self;
        this->event = client.event;
        this->timeout = client.timeout;
        this->keepAlive = client.keepAlive;
        this->information = std::move(client.information);
        this->sendBuffer = std::move(client.sendBuffer);
        this->receiveBuffer = std::move(client.receiveBuffer);
        client.self = -1;
        client.event = 0;
        client.timeout = 0;
        client.keepAlive = false;
    }
    return *this;
}

auto Client::send(const std::source_location &sourceLocation) -> void {
    this->event = this->keepAlive ? EPOLLIN : 0;

    string_view data {this->sendBuffer.read()};
    ssize_t allSentBytes {0};

    while (allSentBytes < data.size()) {
        ssize_t sentBytes {::write(this->self, data.data() + allSentBytes, data.size() - allSentBytes)};
        if (sentBytes > 0)
            allSentBytes += sentBytes;
        else if (sentBytes == 0) {
            event = 0;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                event = EPOLLOUT;
                Log::add(sourceLocation, Level::INFO, " too much data can not be sent to " + this->information);
            } else {
                event = 0;
                Log::add(sourceLocation, Level::ERROR, this->information + " send error: " + strerror(errno));
            }

            break;
        }
    }

    if (allSentBytes > 0)
        this->sendBuffer.adjustRead(allSentBytes);
}

auto Client::receive(const std::source_location &sourceLocation) -> void {
    this->event = EPOLLOUT;

    ssize_t allReceivedBytes {0};

    while (true) {
        auto data {this->receiveBuffer.writableData()};
        ssize_t receivedBytes {::read(this->self, data.first, data.second)};
        if (receivedBytes > 0) {
            allReceivedBytes += receivedBytes;
            this->receiveBuffer.adjustWrite(receivedBytes);
        } else if (receivedBytes == 0) {
            event = 0;
            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                event = 0;
                Log::add(sourceLocation, Level::ERROR, this->information + " receive error: " + strerror(errno));
            }

            break;
        }
    }
}

auto Client::write(const string_view &data) -> void {
    this->sendBuffer.write(data);
}

auto Client::read() -> string {
    string data {this->receiveBuffer.read()};
    this->receiveBuffer.adjustRead(data.size());

    return data;
}

auto Client::get() const -> int {
    return this->self;
}

auto Client::getEvent() const -> uint32_t {
    return this->event;
}

auto Client::setKeepAlive() -> void {
    this->keepAlive = true;
}

auto Client::getTimeout() const -> unsigned short {
    return this->timeout;
}

auto Client::getInformation() const -> string_view {
    return this->information;
}

Client::~Client() {
    if (this->self != -1) {
        if (close(this->self) == -1)
            Log::add(source_location::current(), Level::ERROR, this->information + " close error: "
                    + strerror(errno));
        else
            Log::add(source_location::current(), Level::INFO, this->information + " is closed");
    }
}
