#include "Server.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"

#include <arpa/inet.h>

#include <cstring>

using namespace std;

auto Server::create(unsigned short port) -> unsigned int {
    const int fileDescriptor{Server::socket(AF_INET, SOCK_STREAM, 0)};

    const int optionValue{1};
    Server::setSocketOption(fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, as_bytes(span{&optionValue, 1}));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    Server::translateIpAddress(AF_INET, "127.0.0.1", as_writable_bytes(span{&address.sin_addr, 1}));

    Server::bind(fileDescriptor, reinterpret_cast<__CONST_SOCKADDR_ARG>(&address), sizeof(address));

    Server::listen(fileDescriptor, SOMAXCONN);

    return static_cast<unsigned int>(fileDescriptor);
}

Server::Server(unsigned int fileDescriptorIndex) noexcept : fileDescriptorIndex{fileDescriptorIndex} {}

Server::Server(Server &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, awaiter{std::move(other.awaiter)} {}

auto Server::socket(int domain, int type, int protocol, source_location sourceLocation) -> int {
    const int fileDescriptor{::socket(domain, type, protocol)};

    if (fileDescriptor == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};

    return fileDescriptor;
}

auto Server::setSocketOption(int fileDescriptor, int level, int optionName, span<const byte> optionValue,
                             std::source_location sourceLocation) -> void {
    if (setsockopt(fileDescriptor, level, optionName, optionValue.data(), optionValue.size()) == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};
}

auto Server::translateIpAddress(int domain, string_view ipAddress, span<byte> buffer, source_location sourceLocation)
        -> void {
    if (inet_pton(domain, ipAddress.data(), buffer.data()) != 1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};
}

auto Server::bind(int fileDescriptor, __CONST_SOCKADDR_ARG address, socklen_t addressLength,
                  source_location sourceLocation) -> void {
    if (::bind(fileDescriptor, address, addressLength) == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};
}

auto Server::listen(int fileDescriptor, int number, source_location sourceLocation) -> void {
    if (::listen(fileDescriptor, number) == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};
}

auto Server::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Server::startAccept(io_uring_sqe *sqe) const noexcept -> void {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), nullptr, nullptr, 0};

    const UserData userData{EventType::Accept, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), IORING_ASYNC_CANCEL_ALL};

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

auto Server::setResult(pair<int, unsigned int> result) noexcept -> void { this->awaiter.setResult(result); }
