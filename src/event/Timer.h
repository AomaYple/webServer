#pragma once

#include "../socket/Client.h"

class Timer {
public:
    explicit Timer(const std::shared_ptr<UserRing> &userRing);

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

private:
    [[nodiscard]] static auto create(std::source_location sourceLocation = std::source_location::current()) -> int;

    auto setTime(std::source_location sourceLocation = std::source_location::current()) const -> void;

public:
    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    auto setFileDescriptor(int newFileDescriptor) noexcept -> void;

    auto startTiming() -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client, std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto exist(int clientFileDescriptor) const -> bool;

    auto pop(int clientFileDescriptor) -> Client;

    ~Timer();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    int fileDescriptor;
    std::shared_ptr<UserRing> userRing;
    std::uint_fast8_t now;
    std::uint64_t expireCount;
    std::array<std::unordered_map<int, Client>, 61> wheel;
    std::unordered_map<int, std::uint_least8_t> location;
};
