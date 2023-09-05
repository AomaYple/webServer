#include "Server.h"

#include "../base/Submission.h"
#include "../base/UserData.h"

#include <arpa/inet.h>

#include <exception>

using namespace std;

auto Server::create(unsigned short port) noexcept -> unsigned int {
    const unsigned int fileDescriptor{Server::socket()};

    Server::setSocketOption(fileDescriptor);

    Server::bind(fileDescriptor, port);

    Server::listen(fileDescriptor);

    return fileDescriptor;
}

Server::Server(unsigned int fileDescriptorIndex) noexcept : fileDescriptorIndex{fileDescriptorIndex} {}

Server::Server(Server &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, awaiter{std::move(other.awaiter)} {}

auto Server::socket() noexcept -> unsigned int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};

    if (fileDescriptor == -1) terminate();

    return fileDescriptor;
}

auto Server::setSocketOption(unsigned int fileDescriptor) noexcept -> void {
    const int option{1};
    if (setsockopt(static_cast<int>(fileDescriptor), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option,
                   sizeof(option)) == -1)
        terminate();
}

auto Server::bind(unsigned int fileDescriptor, unsigned short port) noexcept -> void {
    sockaddr_in address{};
    const socklen_t addressLength{sizeof(address)};

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) terminate();

    if (::bind(static_cast<int>(fileDescriptor), reinterpret_cast<sockaddr *>(&address), addressLength) == -1)
        terminate();
}

auto Server::listen(unsigned int fileDescriptor) noexcept -> void {
    if (::listen(static_cast<int>(fileDescriptor), SOMAXCONN) == -1) terminate();
}

auto Server::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Server::setResult(pair<int, unsigned int> result) noexcept -> void { this->awaiter.setResult(result); }

auto Server::startAccept(io_uring_sqe *sqe) const noexcept -> void {
    const Submission submission{sqe, this->fileDescriptorIndex, nullptr, nullptr, 0};

    const UserData userData{EventType::Accept, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{EventType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Server::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{EventType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}
