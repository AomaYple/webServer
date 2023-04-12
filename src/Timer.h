#pragma once

#include "Client.h"

#include <array>
#include <unordered_map>
#include <memory>

class Timer {
public:
    explicit Timer(const std::source_location &sourceLocation = std::source_location::current());

    Timer(const Timer &timer) = delete;

    Timer(Timer &&timer) noexcept;

    auto operator=(Timer &&timer) noexcept -> Timer &;

    auto add(const std::shared_ptr<Client> &client) -> void;

    auto find(int fileDescriptor) -> std::shared_ptr<Client>;

    auto reset(const std::shared_ptr<Client> &client) -> void;

    auto remove(const std::shared_ptr<Client> &client) -> void;

    auto handleReadableEvent(const std::source_location &sourceLocation = std::source_location::current()) -> void;

    auto get() const -> int;

    ~Timer();
private:
    int self;
    unsigned short now;
    std::array<std::unordered_map<int, std::shared_ptr<Client>>, 61> wheel;
    std::unordered_map<int, unsigned short> location;
};
