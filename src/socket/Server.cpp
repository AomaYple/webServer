#include "Server.hpp"

#include "../log/Log.hpp"
#include "../userRing/Submission.hpp"
#include "../userRing/UserData.hpp"
#include "SystemCallError.hpp"

#include <arpa/inet.h>

#include <cstring>

auto Server::create(unsigned short port) -> unsigned int {
    const int fileDescriptor{Server::socket(AF_INET, SOCK_STREAM, 0)};

    const int optionValue{1};
    Server::setSocketOption(fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                            std::as_bytes(std::span{&optionValue, 1}));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    Server::translateIpAddress(AF_INET, "127.0.0.1", std::as_writable_bytes(std::span{&address.sin_addr, 1}));

    Server::bind(fileDescriptor, reinterpret_cast<const sockaddr *>(&address), sizeof(address));

    Server::listen(fileDescriptor, SOMAXCONN);

    return static_cast<unsigned int>(fileDescriptor);
}

Server::Server(unsigned int fileDescriptorIndex) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, acceptTask{nullptr}, cancelTask{nullptr}, closeTask{nullptr} {}

Server::Server(Server &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, acceptTask{std::move(other.acceptTask)},
      cancelTask{std::move(other.cancelTask)}, closeTask{std::move(other.closeTask)},
      awaiter{std::move(other.awaiter)} {}

auto Server::socket(int domain, int type, int protocol, std::source_location sourceLocation) -> int {
    const int fileDescriptor{::socket(domain, type, protocol)};

    if (fileDescriptor == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};

    return fileDescriptor;
}

auto Server::setSocketOption(int fileDescriptor, int level, int optionName, std::span<const std::byte> optionValue,
                             std::source_location sourceLocation) -> void {
    if (setsockopt(fileDescriptor, level, optionName, optionValue.data(), optionValue.size()) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::translateIpAddress(int domain, std::string_view ipAddress, std::span<std::byte> buffer,
                                std::source_location sourceLocation) -> void {
    if (inet_pton(domain, ipAddress.data(), buffer.data()) != 1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::bind(int fileDescriptor, const sockaddr *address, socklen_t addressLength,
                  std::source_location sourceLocation) -> void {
    if (::bind(fileDescriptor, address, addressLength) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::listen(int fileDescriptor, int number, std::source_location sourceLocation) -> void {
    if (::listen(fileDescriptor, number) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Server::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Server::startAccept(io_uring_sqe *sqe) const noexcept -> void {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), nullptr, nullptr, 0};

    const UserData userData{TaskType::Accept, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::setAcceptTask(Task &&task) noexcept -> void { this->acceptTask = std::move(task); }

auto Server::resumeAccept(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->acceptTask.resume();
}

auto Server::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), IORING_ASYNC_CANCEL_ALL};

    const UserData userData{TaskType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Server::setCancelTask(Task &&task) noexcept -> void { this->cancelTask = std::move(task); }

auto Server::resumeCancel(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelTask.resume();
}

auto Server::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{TaskType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}

auto Server::setCloseTask(Task &&task) noexcept -> void { this->closeTask = std::move(task); }

auto Server::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeTask.resume();
}
