#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <netinet/in.h>
#include <source_location>

class Submission;

class Server {
public:
    explicit Server(int fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    auto operator=(const Server &) -> Server & = delete;

    Server(Server &&) = default;

    auto operator=(Server &&) -> Server & = delete;

    ~Server() = default;

    [[nodiscard]] static auto create(unsigned short port) noexcept -> int;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> int;

    auto setAwaiterOutcome(Outcome outcome) noexcept -> void;

    auto setAcceptGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getAcceptSubmission() const noexcept -> Submission;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    auto resumeAccept() const -> void;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCancelSubmission() const noexcept -> Submission;

    [[nodiscard]] auto cancel() const noexcept -> const Awaiter &;

    auto resumeCancel() const -> void;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCloseSubmission() const noexcept -> Submission;

    [[nodiscard]] auto close() const noexcept -> const Awaiter &;

    auto resumeClose() const -> void;

private:
    [[nodiscard]] static auto socket(std::source_location sourceLocation = std::source_location::current()) noexcept
            -> int;

    static auto setSocketOption(int fileDescriptor,
                                std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    static auto translateIpAddress(in_addr &address,
                                   std::source_location sourceLocation = std::source_location::current()) noexcept
            -> void;

    static auto bind(int fileDescriptor, const sockaddr_in &address,
                     std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    static auto listen(int fileDescriptor,
                       std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    const int fileDescriptorIndex;
    Generator acceptGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
