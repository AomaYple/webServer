#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"
#include "FileDescriptor.hpp"

class Timer : public FileDescriptor {
public:
    [[nodiscard]] static auto create() -> int;

    Timer(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept;

    Timer(const Timer &) = delete;

    auto operator=(const Timer &) -> Timer & = delete;

    Timer(Timer &&) = default;

    auto operator=(Timer &&) -> Timer & = default;

    ~Timer() = default;

    auto setGenerator(Generator &&newGenerator) noexcept -> void;

    auto resumeGenerator(Outcome outcome) -> void;

    [[nodiscard]] auto timing() -> const Awaiter &;

    auto add(int fileDescriptor, unsigned long seconds) -> void;

    auto update(int fileDescriptor, unsigned long seconds) -> void;

    auto remove(int fileDescriptor) -> void;

    [[nodiscard]] auto clearTimeout() -> std::vector<int>;

private:
    [[nodiscard]] static auto
    createTimerFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> int;

    static auto setTime(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
            -> void;

    unsigned long expireCount{}, now{};
    std::array<std::unordered_map<int, unsigned long>, 65> wheel;
    std::unordered_map<int, unsigned long> location;
    Generator generator{nullptr};
    Awaiter awaiter{};
};
