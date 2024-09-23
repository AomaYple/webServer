#pragma once

#include "FileDescriptor.hpp"

#include <chrono>
#include <unordered_map>

class Timer final : public FileDescriptor {
public:
    [[nodiscard]] static auto create() -> int;

    explicit Timer(int fileDescriptor) noexcept;

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept = default;

    auto operator=(const Timer &) -> Timer & = delete;

    constexpr auto operator=(Timer &&) noexcept -> Timer & = delete;

    ~Timer() override = default;

    [[nodiscard]] auto timing() noexcept -> Awaiter;

    auto add(int fileDescriptor, std::chrono::seconds seconds) -> void;

    auto update(int fileDescriptor, std::chrono::seconds seconds) -> void;

    auto remove(int fileDescriptor) -> void;

    [[nodiscard]] auto clearTimeout() -> std::vector<int>;

private:
    unsigned long timeout{};
    std::chrono::seconds now{};
    std::array<std::unordered_map<int, unsigned long>, 61> wheel;
    std::unordered_map<int, std::chrono::seconds> location;
};
