#pragma once

#include <liburing.h>

#include <cstdint>

class Submission {
public:
    explicit Submission(io_uring_sqe *sqe) noexcept;

    Submission(const Submission &) = delete;

    auto setUserData(std::uint_fast64_t userData) noexcept -> void;

    auto setFlags(std::uint_fast32_t flags) noexcept -> void;

    auto setBufferGroup(std::uint_fast16_t bufferGroup) noexcept -> void;

    auto accept(std::int_fast32_t fileDescriptor, sockaddr *address, socklen_t *addressLength,
                std::int_fast32_t flags) noexcept -> void;

    auto read(std::int_fast32_t fileDescriptor, void *buffer, std::uint_fast32_t bufferLength,
              std::uint_fast64_t offset) noexcept -> void;

    auto receive(std::int_fast32_t fileDescriptor, void *buffer, std::uint_fast64_t bufferLength,
                 std::int_fast32_t flags) noexcept -> void;

    auto send(std::int_fast32_t fileDescriptor, const void *buffer, std::uint_fast64_t bufferLength,
              std::int_fast32_t flags, std::uint_fast32_t zeroCopyFlags) noexcept -> void;

    auto cancel(std::int_fast32_t fileDescriptor, std::uint_fast32_t flags) noexcept -> void;

    auto close(std::uint_fast32_t fileDescriptorIndex) noexcept -> void;

private:
    io_uring_sqe *submission;
};
