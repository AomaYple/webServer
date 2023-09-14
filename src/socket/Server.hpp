#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Task.hpp"

#include <liburing.h>

#include <source_location>
#include <span>
#include <string_view>

class Server {
public:
    [[nodiscard]] static auto create(unsigned short port) -> unsigned int;

    explicit Server(unsigned int fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

private:
    [[nodiscard]] static auto socket(int domain, int type, int protocol,
                                     std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setSocketOption(int fileDescriptor, int level, int optionName, std::span<const std::byte> optionValue,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto translateIpAddress(int domain, std::string_view ipAddress, std::span<std::byte> buffer,
                                   std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(int fileDescriptor, const sockaddr *address, socklen_t addressLength,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(int fileDescriptor, int number,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto startAccept(io_uring_sqe *sqe) const noexcept -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    auto setAcceptTask(Task &&task) noexcept -> void;

    auto resumeAccept(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int, unsigned int> result) -> void;

private:
    const unsigned int fileDescriptorIndex;
    Task acceptTask, cancelTask, closeTask;
    Awaiter awaiter;
};
