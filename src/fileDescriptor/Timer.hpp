#pragma once

#include "FileDescriptor.hpp"

#include <chrono>

class Timer : public FileDescriptor {
public:
    [[nodiscard]] static auto create() -> int;

    Timer(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept;

    Timer(const Timer &) = delete;

    auto operator=(const Timer &) -> Timer & = delete;

    Timer(Timer &&) noexcept = default;

    auto operator=(Timer &&) noexcept -> Timer & = default;

    ~Timer() = default;

    [[nodiscard]] auto clearTimeout() -> std::vector<int>;

    auto add(int fileDescriptor, std::chrono::seconds seconds) -> void;

    auto update(int fileDescriptor, std::chrono::seconds seconds) -> void;

    auto remove(int fileDescriptor) -> void;

    auto timing() -> void;

private:
    [[nodiscard]] static auto
    createTimerFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setTime(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
            -> void;

    unsigned long expireCount{};
    std::chrono::seconds now{};
    std::array<std::unordered_map<int, std::chrono::seconds>, 61> wheel;
    std::unordered_map<int, std::chrono::seconds> location;
};
