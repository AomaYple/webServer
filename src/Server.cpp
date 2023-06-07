#include "Server.h"

#include <cstring>

#include "Event.h"
#include "Log.h"
#include "Submission.h"

using std::string, std::shared_ptr, std::runtime_error, std::source_location;

thread_local bool Server::instance{false};

Server::Server(unsigned short port, shared_ptr<Ring> &ring) : self{socket(AF_INET, SOCK_STREAM, 0)} {
    if (Server::instance) throw runtime_error("server one thread one instance");
    Server::instance = true;

    if (this->self == -1) throw runtime_error("server create error: " + string{std::strerror(errno)});

    int option{1};
    if (setsockopt(this->self, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw runtime_error("server reuse address and reuse port error: " + string{std::strerror(errno)});

    sockaddr_in address{};
    socklen_t addressLength{sizeof(address)};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1)
        throw runtime_error("server translate ipAddress error: " + string{std::strerror(errno)});

    if (bind(this->self, reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        throw runtime_error("server bind error: " + string{std::strerror(errno)});

    if (listen(this->self, SOMAXCONN) == -1)
        throw runtime_error("server listen error: " + string{std::strerror(errno)});

    this->accept(ring);
}

Server::Server(Server &&server) noexcept: self{server.self} { server.self = -1; }

auto Server::operator=(Server &&server) noexcept -> Server & {
    if (this != &server) {
        this->self = server.self;
        server.self = -1;
    }
    return *this;
}

auto Server::accept(shared_ptr<Ring> &ring) const -> void {
    Submission submission{ring};

    Event event{Type::ACCEPT, this->self};
    submission.setData(reinterpret_cast<unsigned long long &>(event));
    submission.accept(this->self, nullptr, nullptr, 0);
}

Server::~Server() {
    int returnValue{close(this->self)};

    if (returnValue == 0)
        Log::add(source_location::current(), Level::INFO, "server is closed");
    else
        Log::add(source_location::current(), Level::ERROR, std::strerror(errno));

    Server::instance = false;
}
