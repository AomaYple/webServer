#include "Server.hpp"

#include "../log/Exception.hpp"
#include "../userRing/Event.hpp"
#include "../userRing/Submission.hpp"

#include <arpa/inet.h>

#include <cstring>

Server::Server(unsigned int fileDescriptorIndex) noexcept : fileDescriptorIndex{fileDescriptorIndex}, awaiter{} {}

auto Server::create(unsigned short port) -> unsigned int {
    const unsigned int fileDescriptor{Server::socket()};

    Server::setSocketOption(fileDescriptor);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    Server::translateIpAddress(address.sin_addr);

    Server::bind(fileDescriptor, address);

    Server::listen(fileDescriptor);

    return fileDescriptor;
}

auto Server::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Server::startAccept(io_uring_sqe *sqe) const noexcept -> void {
    const Submission submission{sqe, this->fileDescriptorIndex, nullptr, nullptr, 0};

    const Event event{Event::Type::accept, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::setAcceptGenerator(Generator &&generator) noexcept -> void {
    this->acceptGenerator = std::move(generator);
}

auto Server::resumeAccept(Outcome result) -> void {
    this->awaiter.set_result(result);

    this->acceptGenerator.resume();
}

auto Server::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const Event event{Event::Type::cancel, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Server::setCancelGenerator(Generator &&generator) noexcept -> void {
    this->cancelGenerator = std::move(generator);
}

auto Server::resumeCancel(Outcome result) -> void {
    this->awaiter.set_result(result);

    this->cancelGenerator.resume();
}

auto Server::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const Event event{Event::Type::close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    return this->awaiter;
}

auto Server::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Server::resumeClose(Outcome result) -> void {
    this->awaiter.set_result(result);

    this->closeGenerator.resume();
}

auto Server::socket(std::source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1) throw Exception{Log{Log::Level::fatal, sourceLocation, std::strerror(errno)}};

    return fileDescriptor;
}

auto Server::setSocketOption(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    constexpr int option{1};
    if (setsockopt(static_cast<int>(fileDescriptor), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option,
                   sizeof(option)) == -1)
        throw Exception{Log{Log::Level::fatal, sourceLocation, std::strerror(errno)}};
}

auto Server::translateIpAddress(in_addr &address, std::source_location sourceLocation) -> void {
    if (inet_pton(AF_INET, "127.0.0.1", &address) != 1)
        throw Exception{Log{Log::Level::fatal, sourceLocation, std::strerror(errno)}};
}

auto Server::bind(unsigned int fileDescriptor, const sockaddr_in &address, std::source_location sourceLocation)
        -> void {
    if (::bind(static_cast<int>(fileDescriptor), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
        throw Exception{Log{Log::Level::fatal, sourceLocation, std::strerror(errno)}};
}

auto Server::listen(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    if (::listen(static_cast<int>(fileDescriptor), SOMAXCONN) == -1)
        throw Exception{Log{Log::Level::fatal, sourceLocation, std::strerror(errno)}};
}
