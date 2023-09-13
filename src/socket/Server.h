#pragma once

#include "../coroutine/Awaiter.h"
#include "../coroutine/Task.h"

#include <liburing.h>

#include <source_location>
#include <span>
#include <string_view>

class Server {
public:
    [[nodiscard]] static auto create(uint16_t port) -> uint32_t;

    explicit Server(uint32_t fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

private:
    [[nodiscard]] static auto socket(int domain, int type, int protocol,
                                     std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setSocketOption(int fileDescriptor, int level, int optionName, std::span<const std::byte> optionValue,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto translateIpAddress(int domain, std::string_view ipAddress, std::span<std::byte> buffer,
                                   std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(int fileDescriptor, __CONST_SOCKADDR_ARG address, socklen_t addressLength,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(int fileDescriptor, int number,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> uint32_t;

    auto startAccept(io_uring_sqe *sqe) const noexcept -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    auto setAcceptTask(Task &&task) noexcept -> void;

    auto resumeAccept(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int32_t, uint32_t> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int32_t, uint32_t> result) -> void;

private:
    const uint32_t fileDescriptorIndex;
    Task acceptTask, cancelTask, closeTask;
    Awaiter awaiter;
};
