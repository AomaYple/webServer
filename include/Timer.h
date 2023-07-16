#pragma once

#include "Client.h"
#include "Submission.h"

class Timer {
public:
    Timer();

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

    auto operator=(Timer &&) noexcept -> Timer &;

    auto start(Submission &&submission) noexcept -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client) -> void;

    auto exist(int socket) const noexcept -> bool;

    auto pop(int socket) -> Client;

    ~Timer();

private:
    auto close() const -> void;

    int self;
    unsigned short now;
    unsigned long expireCount;
    std::array<std::unordered_map<int, Client>, 61> wheel;
    std::unordered_map<int, unsigned short> location;
};
