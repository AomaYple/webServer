#include "Server.h"

#include <arpa/inet.h>

#include <cstring>

#include "Event.h"
#include "Submission.h"

using std::string, std::shared_ptr, std::runtime_error;

Server::Server(unsigned short port, const shared_ptr<Ring> &ring)
    : self{socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)}, ring{ring} {
    if (this->self == -1) throw runtime_error("server create error: " + string{std::strerror(errno)});

    this->setSocketOption();

    this->bind(port);

    this->listen();

    this->ring->updateFileDescriptor(this->self);

    this->accept();
}

Server::Server(Server &&other) noexcept : self{other.self}, ring{std::move(other.ring)} { other.self = -1; }

auto Server::operator=(Server &&other) noexcept -> Server & {
    if (this != &other) {
        this->self = other.self;
        this->ring = std::move(other.ring);
        other.self = -1;
    }
    return *this;
}

auto Server::accept() -> void {
    Submission submission{*this->ring};

    Event event{Type::ACCEPT, this->self};

    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.setFlags(IOSQE_FIXED_FILE);

    submission.accept(this->self, nullptr, nullptr, SOCK_NONBLOCK);
}

Server::~Server() {
    if (this->self != -1) {
        Submission submission{*this->ring};

        Event event{Type::CLOSE, this->self};

        submission.setData(reinterpret_cast<unsigned long long &>(event));
        submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);

        submission.close(this->self);
    }
}

auto Server::setSocketOption() const -> void {
    int option{1};
    if (setsockopt(this->self, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw runtime_error("server reuse address and reuse port error: " + string{std::strerror(errno)});
}

auto Server::bind(unsigned short port) const -> void {
    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw runtime_error("server translate ipAddress error: " + string{std::strerror(errno)});

    if (::bind(this->self, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw runtime_error("server bind error: " + string{std::strerror(errno)});
}

auto Server::listen() const -> void {
    if (::listen(this->self, SOMAXCONN) == -1)
        throw runtime_error("server listen error: " + string{std::strerror(errno)});
}
