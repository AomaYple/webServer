#include "Client.h"
#include "Log.h"

#include <cstring>

#include <sys/socket.h>
#include <sys/epoll.h>

using std::string, std::string_view, std::to_string, std::source_location;

Client::Client(int fileDescriptor, std::string information) : self(fileDescriptor), timeout(0), information(std::move(information)) {}

Client::Client(Client &&client) noexcept : self(client.self), timeout(0), information(std::move(client.information)),
        sendBuffer(std::move(client.sendBuffer)), receiveBuffer(std::move(client.receiveBuffer)) {
    client.self = -1;
    client.timeout = 0;
}

auto Client::operator=(Client &&client) noexcept -> Client & {
    if (this != &client) {
        this->self = client.self;
        this->timeout = client.timeout;
        this->information = std::move(client.information);
        this->sendBuffer = std::move(client.sendBuffer);
        this->receiveBuffer = std::move(client.receiveBuffer);
        client.self = -1;
        client.timeout = 0;
    }
    return *this;
}

auto Client::send(std::source_location sourceLocation) -> uint32_t {
    uint32_t event {this->timeout == 0 ? 0 : EPOLLET | EPOLLRDHUP | EPOLLIN};

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
                event = EPOLLET | EPOLLOUT;
                Log::add(sourceLocation, Level::WARN, " too much data can not be sent to " + this->information);
            } else {
                event = 0;
                Log::add(sourceLocation, Level::ERROR, this->information + " send error: " + strerror(errno));
            }

            break;
        }
    }

    if (allSentBytes > 0)
        this->sendBuffer.adjustRead(allSentBytes);

    return event;
}

auto Client::receive(std::source_location sourceLocation) -> uint32_t {
    uint32_t event {EPOLLET | EPOLLOUT};

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

    return event;
}

auto Client::write(const std::string_view &data) -> void {
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

auto Client::getInformation() const -> string_view {
    return this->information;
}

auto Client::getExpire() const -> unsigned int {
    return this->timeout;
}

auto Client::setExpire(unsigned int time) -> void {
    this->timeout = time;
}

Client::~Client() {
    if (this->self != -1) {
        if (shutdown(this->self, SHUT_RDWR) == -1)
            Log::add(source_location::current(), Level::ERROR, this->information + " shutdown error: " + strerror(errno));

        if (close(this->self) == -1)
            Log::add(source_location::current(), Level::ERROR, this->information + " close error: " + strerror(errno));
        else
            Log::add(source_location::current(), Level::INFO, this->information + " is closed");
    }
}
