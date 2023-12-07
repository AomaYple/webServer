#include "Server.hpp"

#include "../log/Log.hpp"
#include "../log/logger.hpp"
#include "../userRing/Submission.hpp"

#include <arpa/inet.h>
#include <liburing.h>

#include <cstring>

Server::Server(int fileDescriptorIndex) noexcept : fileDescriptorIndex{fileDescriptorIndex}, awaiter{} {}

auto Server::create(unsigned short port) noexcept -> int {
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

auto Server::getFileDescriptorIndex() const noexcept -> int { return this->fileDescriptorIndex; }

auto Server::setAwaiterOutcome(Outcome outcome) noexcept -> void { this->awaiter.setOutcome(outcome); }

auto Server::setAcceptGenerator(Generator &&generator) noexcept -> void {
    this->acceptGenerator = std::move(generator);
}

auto Server::getAcceptSubmission() const noexcept -> Submission {
    return {Event{Event::Type::accept, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
            Submission::AcceptParameters{nullptr, nullptr, 0}};
}

auto Server::accept() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::resumeAccept() const -> void { this->acceptGenerator.resume(); }

auto Server::setCancelGenerator(Generator &&generator) noexcept -> void {
    this->cancelGenerator = std::move(generator);
}

auto Server::getCancelSubmission() const noexcept -> Submission {
    return {Event{Event::Type::cancel, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
            Submission::CancelParameters{IORING_ASYNC_CANCEL_ALL}};
}

auto Server::cancel() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::resumeCancel() const -> void { this->cancelGenerator.resume(); }

auto Server::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Server::getCloseSubmission() const noexcept -> Submission {
    return {Event{Event::Type::close, this->fileDescriptorIndex}, 0, Submission::CloseParameters{}};
}

auto Server::close() const noexcept -> const Awaiter & { return this->awaiter; }

auto Server::resumeClose() const -> void { this->closeGenerator.resume(); }

auto Server::socket(std::source_location sourceLocation) noexcept -> int {
    const int fileDescriptor{::socket(AF_INET, SOCK_STREAM, 0)};
    if (fileDescriptor == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }

    return fileDescriptor;
}

auto Server::setSocketOption(int fileDescriptor, std::source_location sourceLocation) noexcept -> void {
    static constexpr int option{1};
    if (setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Server::translateIpAddress(in_addr &address, std::source_location sourceLocation) noexcept -> void {
    if (inet_pton(AF_INET, "127.0.0.1", &address) != 1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Server::bind(int fileDescriptor, const sockaddr_in &address, std::source_location sourceLocation) noexcept
        -> void {
    if (::bind(fileDescriptor, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }
}

auto Server::listen(int fileDescriptor, std::source_location sourceLocation) noexcept -> void {
    if (::listen(fileDescriptor, SOMAXCONN) == -1) {
        logger::push(Log{Log::Level::fatal, std::strerror(errno), sourceLocation});
        logger::flush();

        std::terminate();
    }
}
