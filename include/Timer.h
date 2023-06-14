#pragma once

#include <array>
#include <memory>
#include <unordered_map>

#include "Client.h"

class Timer {
public:
    Timer();

    Timer(const Timer &timer) = delete;

    Timer(Timer &&other) noexcept;

    auto operator=(Timer &&other) noexcept -> Timer &;

    auto add(const std::shared_ptr<Client> &client) -> void;

    auto find(int socket) -> std::shared_ptr<Client>;

    auto reset(const std::shared_ptr<Client> &client) -> void;

    auto remove(const std::shared_ptr<Client> &client) -> void;

    auto handleTimeout(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto get() const -> int;

    ~Timer();

private:
    int fileDescriptor;
    unsigned short now;
    std::unordered_map<int, unsigned short> location;
    std::array<std::unordered_map<int, std::shared_ptr<Client>>, 61> wheel;
};