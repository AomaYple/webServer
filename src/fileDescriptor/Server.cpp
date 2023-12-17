#include "Server.hpp"

#include "../log/Exception.hpp"
#include "../ring/Submission.hpp"

#include <arpa/inet.h>

#include <cstring>

auto Server::create(unsigned short port) -> int {
    const int fileDescriptor{Server::socket()};
    Server::setSocketOption(fileDescriptor);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    Server::translateIpAddress(address.sin_addr);
    Server::bind(fileDescriptor, address);
    Server::listen(fileDescriptor);

    return fileDescriptor;
}

Server::Server(int fileDescriptor, std::shared_ptr<Ring> ring) : FileDescriptor(fileDescriptor, std::move(ring)) {}

auto Server::accept() const -> void {
    const Submission submission{Event{Event::Type::accept, this->getFileDescriptor()}, IOSQE_FIXED_FILE,
                                Submission::AcceptParameters{nullptr, nullptr, 0}};

    this->getRing()->submit(submission);
}

auto Server::socket(std::source_location sourceLocation) -> int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};
    if (fileDescriptor == -1) throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    return fileDescriptor;
}

auto Server::setSocketOption(int fileDescriptor, std::source_location sourceLocation) -> void {
    static constexpr int option{1};
    if (setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

auto Server::translateIpAddress(in_addr &address, std::source_location sourceLocation) -> void {
    if (inet_pton(AF_INET, "127.0.0.1", &address) != 1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

auto Server::bind(int fileDescriptor, const sockaddr_in &address, std::source_location sourceLocation) -> void {
    if (::bind(fileDescriptor, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}

auto Server::listen(int fileDescriptor, std::source_location sourceLocation) -> void {
    if (::listen(fileDescriptor, SOMAXCONN) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}
