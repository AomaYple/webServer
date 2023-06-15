#pragma once

#include <array>
#include <memory>
#include <unordered_map>

#include "Client.h"

class Timer {
public:
    Timer();

    Timer(const Timer &other) = delete;

    Timer(Timer &&other) noexcept;

    auto operator=(Timer &&other) noexcept -> Timer &;

    auto get() const -> int;

    auto add(const std::shared_ptr<Client> &client) -> void;

    auto find(int socket) -> std::shared_ptr<Client>;

    auto reset(const std::shared_ptr<Client> &client) -> void;

    auto remove(const std::shared_ptr<Client> &client) -> void;

    auto handleExpiration() -> void;

    ~Timer();

private:
    int fileDescriptor;
    unsigned short now;
    std::unordered_map<int, unsigned short> location;
    std::array<std::unordered_map<int, std::shared_ptr<Client>>, 61> wheel;
};