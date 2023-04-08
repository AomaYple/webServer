#include "Server.h"
#include "Log.h"

#include <cstring>

#include <fcntl.h>

using std::string, std::string_view, std::to_string, std::vector, std::shared_ptr, std::make_shared, std::source_location;

Server::Server(unsigned short port, std::source_location sourceLocation) : self(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)),
                                                                           idleFileDescriptor(open("/dev/null", O_RDONLY)), address({}), addressLength(sizeof(address)) {
    if (this->self == -1)
        Log::add(sourceLocation, Level::ERROR, "Server create error: " + string(strerror(errno)));

    int option {1};
    if (setsockopt(this->self, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
        Log::add(sourceLocation, Level::ERROR, "Server reuseAddress error: " + string(strerror(errno)));

    if (setsockopt(this->self, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) == -1)
        Log::add(sourceLocation, Level::ERROR, "Server reusePort error: " + string(strerror(errno)));

    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &this->address.sin_addr) != 1)
        Log::add(sourceLocation, Level::ERROR, "Server translate ipAddress error: " + string(strerror(errno)));

    if (::bind(this->self, reinterpret_cast<sockaddr *>(&this->address), this->addressLength) == -1)
        Log::add(sourceLocation, Level::ERROR, "Server bind error: " + string(strerror(errno)));

    if (::listen(this->self, SOMAXCONN) == -1)
        Log::add(sourceLocation, Level::ERROR, "Server listen error: " + string(strerror(errno)));
}

Server::Server(Server &&server) noexcept : self(server.self), idleFileDescriptor(server.idleFileDescriptor), address(server.address),
                                           addressLength(server.addressLength) {
    server.self = -1;
    server.idleFileDescriptor = -1;
    server.address = {};
    server.addressLength = 0;
}

auto Server::operator=(Server &&server) noexcept -> Server & {
    if (this != &server) {
        this->self = server.self;
        this->idleFileDescriptor = server.idleFileDescriptor;
        this->address = server.address;
        this->addressLength = server.addressLength;
        server.self = -1;
        server.idleFileDescriptor = -1;
        server.address = {};
        server.addressLength = 0;
    }
    return *this;
}

auto Server::accept(source_location sourceLocation) -> vector<shared_ptr<Client>> {
    vector<shared_ptr<Client>> clients;

    while (true) {
        this->address = {};
        this->addressLength = {sizeof(this->address)};
        int fileDescriptor {accept4(this->self, reinterpret_cast<sockaddr *>(&this->address), &this->addressLength,
                                    SOCK_NONBLOCK)};

        if (fileDescriptor != -1) {
            string information(INET_ADDRSTRLEN, 0);

            if (inet_ntop(AF_INET, &this->address.sin_addr, information.data(), information.size()) == nullptr)
                Log::add(sourceLocation,Level::ERROR, "Client translate ipAddress error: " + string(strerror(errno)));

            information += ":" + to_string(ntohs(this->address.sin_port));

            Log::add(sourceLocation, Level::INFO, "new client " + information);

            clients.emplace_back(make_shared<Client>(fileDescriptor, information));
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else if (errno == EMFILE) {
                close(this->idleFileDescriptor);
                this->idleFileDescriptor = ::accept(this->self, nullptr, nullptr);
                close(this->idleFileDescriptor);
                this->idleFileDescriptor = open("/dev/null", O_RDONLY);
            } else {
                Log::add(sourceLocation, Level::ERROR, "Server accept error: " + string(strerror(errno)));
                break;
            }
        }
    }

    return clients;
}

auto Server::get() const -> int {
    return this->self;
}

Server::~Server() {
    if (this->self != -1 && close(this->self) == -1)
        Log::add(source_location::current(), Level::ERROR, "Server close error: " + string(strerror(errno)));
}
