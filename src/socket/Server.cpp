#include "Server.hpp"

#include "../log/Log.hpp"
#include "../userRing/Submission.hpp"
#include "../userRing/UserData.hpp"
#include "SystemCallError.hpp"

#include <arpa/inet.h>

#include <cstring>

Server::Server(unsigned int fileDescriptorIndex) noexcept : fileDescriptorIndex{fileDescriptorIndex} {}

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

auto Server::socket(std::source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};

    return fileDescriptor;
}

auto Server::setSocketOption(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    constexpr int option{1};
    if (setsockopt(static_cast<int>(fileDescriptor), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option,
                   sizeof(option)) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::translateIpAddress(in_addr &address, std::source_location sourceLocation) -> void {
    if (inet_pton(AF_INET, "127.0.0.1", &address) != 1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::bind(unsigned int fileDescriptor, const sockaddr_in &address, std::source_location sourceLocation)
        -> void {
    if (::bind(static_cast<int>(fileDescriptor), reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::listen(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    if (::listen(static_cast<int>(fileDescriptor), SOMAXCONN) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Server::startAccept(io_uring_sqe *sqe) const noexcept -> void {
    const Submission submission{sqe, this->fileDescriptorIndex, nullptr, nullptr, 0};

    const UserData userData{EventType::Accept, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::setAcceptGenerator(Generator &&generator) noexcept -> void {
    this->acceptGenerator = std::move(generator);
}

auto Server::resumeAccept(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->acceptGenerator.resume();
}

auto Server::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{EventType::Close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    return this->awaiter;
}

auto Server::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Server::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeGenerator.resume();
}
