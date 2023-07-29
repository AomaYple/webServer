#pragma once

#include "../network/Client.h"

class Timer {
public:
    Timer();

    Timer(const Timer &) = delete;

    Timer(Timer &&)

            noexcept;

    auto operator=(Timer &&)

            noexcept -> Timer &;

    auto start(io_uring_sqe *sqe)

            noexcept -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client) -> void;

    auto exist(int clientFileDescriptor) const -> bool;

    auto pop(int clientFileDescriptor) -> Client;

    ~Timer();

private:
    auto create() -> void;

    auto setTime() const -> void;

    auto close() const -> void;

    int fileDescriptor;
    unsigned short now;
    unsigned long expireCount;
    std::array<std::unordered_map<int, Client>, 61> wheel;
    std::unordered_map<int, unsigned short> location;
};
