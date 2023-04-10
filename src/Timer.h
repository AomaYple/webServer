#pragma once

#include "Client.h"

#include <array>
#include <unordered_map>
#include <memory>

class Timer {
public:
    explicit Timer(std::source_location sourceLocation = std::source_location::current());

    Timer(const Timer &timer) = delete;

    Timer(Timer &&timer) noexcept;

    auto operator=(Timer &&timer) noexcept -> Timer &;

    auto add(const std::shared_ptr<Client>& client) -> void;

    auto reset(std::shared_ptr<Client> &client) -> void;

    auto remove(std::shared_ptr<Client> &client) -> void;

    [[nodiscard]] auto handleRead(std::source_location sourceLocation = std::source_location::current()) -> std::vector<int>;

    auto get() const -> int;

    ~Timer();
private:
    int self;
    unsigned int now;
    std::array<std::unordered_map<int, std::shared_ptr<Client>>, 3600> wheel;
    std::unordered_map<int ,unsigned int> table;
};
