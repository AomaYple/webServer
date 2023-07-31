#pragma once

#include "../network/Client.h"

class Timer {
public:
    Timer();

    Timer(const Timer &) = delete;

    auto start(io_uring_sqe *sqe) noexcept -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client, std::source_location sourceLocation = std::source_location::current()) -> void;

    auto exist(int clientFileDescriptor) const -> bool;

    auto pop(int clientFileDescriptor) -> Client;

    ~Timer();

private:
    static auto create(std::source_location sourceLocation = std::source_location::current()) -> int;

    auto setTime(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto close(std::source_location sourceLocation = std::source_location::current()) const -> void;

    int fileDescriptor;
    unsigned short now;
    unsigned long expireCount;
    std::array<std::unordered_map<int, Client>, 61> wheel;
    std::unordered_map<int, unsigned short> location;
};
