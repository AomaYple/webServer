#include "Server.h"

#include <arpa/inet.h>

#include <cstring>

#include "Event.h"
#include "Log.h"
#include "Submission.h"

using std::string, std::shared_ptr, std::runtime_error, std::source_location;

Server::Server(unsigned short port, const shared_ptr<Ring> &ring)
    : socket{::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)}, ring{ring} {
    if (this->socket == -1) throw runtime_error("server create error: " + string{std::strerror(errno)});

    this->setSocketOption();

    this->bind(port);

    this->listen();

    this->registerFileDescriptor();
}

Server::Server(Server &&other) noexcept : socket{other.socket}, ring{std::move(other.ring)} { other.socket = -1; }

auto Server::operator=(Server &&other) noexcept -> Server & {
    if (this != &other) {
        this->socket = other.socket;
        this->ring = std::move(other.ring);
        other.socket = -1;
    }
    return *this;
}

Server::~Server() {
    if (this->socket != -1) {}
}

auto Server::setSocketOption() const -> void {
    int option{1};
    if (setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw runtime_error("server reuseAddress and reusePort error: " + string{std::strerror(errno)});
}

auto Server::bind(unsigned short port) const -> void {
    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw runtime_error("server translate ipAddress error: " + string{std::strerror(errno)});

    if (::bind(this->socket, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw runtime_error("server bind error: " + string{std::strerror(errno)});
}

auto Server::listen() const -> void {
    if (::listen(this->socket, SOMAXCONN) == -1)
        throw runtime_error("server listen error: " + string{std::strerror(errno)});
}

auto Server::registerFileDescriptor() -> void {
    int returnValue{io_uring_register_files_update(&this->ring->get(), 0, &this->socket, 1)};

}
