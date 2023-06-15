#include "Server.h"

#include <arpa/inet.h>
#include <fcntl.h>

#include <cstring>

#include "Log.h"

using std::string, std::to_string, std::vector, std::shared_ptr, std::make_shared, std::source_location,
        std::runtime_error;

Server::Server(unsigned short port)
    : socket{::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)}, idleFileDescriptor{open("/dev/null", O_RDONLY)} {
    if (this->socket == -1) throw runtime_error("server create error: " + string{std::strerror(errno)});

    this->setSocketOption();

    this->bind(port);

    this->listen();
}

Server::Server(Server &&other) noexcept : socket{other.socket}, idleFileDescriptor{other.idleFileDescriptor} {
    other.socket = -1;
    other.idleFileDescriptor = -1;
}

auto Server::operator=(Server &&other) noexcept -> Server & {
    if (this != &other) {
        this->socket = other.socket;
        this->idleFileDescriptor = other.idleFileDescriptor;
        other.socket = -1;
        other.idleFileDescriptor = -1;
    }
    return *this;
}

auto Server::get() const -> int { return this->socket; }

auto Server::accept(source_location sourceLocation) -> vector<shared_ptr<Client>> {
    vector<shared_ptr<Client>> clients;

    while (true) {
        sockaddr_in address = {};
        socklen_t addressLength{sizeof(address)};

        int clientSocket{accept4(this->socket, reinterpret_cast<sockaddr *>(&address), &addressLength, SOCK_NONBLOCK)};

        if (clientSocket != -1) {
            string information(INET_ADDRSTRLEN, 0);

            if (inet_ntop(AF_INET, &address.sin_addr, information.data(), information.size()) == nullptr)
                Log::add(sourceLocation, Level::WARN,
                         "translate client ipAddress error: " + string{std::strerror(errno)});

            information += ":" + to_string(ntohs(address.sin_port));

            Log::add(sourceLocation, Level::INFO, "new client " + information);

            clients.emplace_back(make_shared<Client>(clientSocket, std::move(information)));
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EMFILE) {
                close(this->idleFileDescriptor);

                this->idleFileDescriptor = ::accept(this->socket, nullptr, nullptr);

                close(this->idleFileDescriptor);

                this->idleFileDescriptor = open("/dev/null", O_RDONLY);
            } else {
                Log::add(sourceLocation, Level::WARN, "server accept error: " + string{std::strerror(errno)});

                break;
            }
        }
    }

    return clients;
}

Server::~Server() {
    if (this->idleFileDescriptor != -1 && close(this->idleFileDescriptor) == -1)
        Log::add(source_location::current(), Level::ERROR,
                 "server close idleFileDescriptor error: " + string{std::strerror(errno)});

    if (this->socket != -1 && close(this->socket) == -1)
        Log::add(source_location::current(), Level::ERROR, "server close error: " + string{std::strerror(errno)});
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
