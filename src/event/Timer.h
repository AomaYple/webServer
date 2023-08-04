#pragma once

#include "../network/Client.h"

class Timer {
public:
    Timer();

    Timer(const Timer &) = delete;

    auto start(io_uring_sqe *sqe) noexcept -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto exist(std::int_fast32_t clientFileDescriptor) const -> bool;

    auto pop(std::int_fast32_t clientFileDescriptor) -> Client;

    ~Timer();

private:
    static auto create(std::source_location sourceLocation = std::source_location::current()) -> std::int_fast32_t;

    auto setTime(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto close(std::source_location sourceLocation = std::source_location::current()) const -> void;

    std::int_fast32_t fileDescriptor;
    std::uint_fast16_t now;
    std::uint_fast64_t expireCount;
    std::array<std::unordered_map<std::int_fast32_t, Client>, 61> wheel;
    std::unordered_map<std::int_fast32_t, std::uint_fast16_t> location;
};
