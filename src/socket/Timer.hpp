#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"

#include <array>
#include <source_location>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Submission;

class Timer {
public:
    explicit Timer(int fileDescriptorIndex) noexcept;

    Timer(const Timer &) = delete;

    auto operator=(const Timer &) -> Timer & = delete;

    Timer(Timer &&) = default;

    auto operator=(Timer &&) -> Timer & = delete;

    ~Timer() = default;

    [[nodiscard]] static auto create() noexcept -> int;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> int;

    [[nodiscard]] auto clearTimeout() noexcept -> std::vector<int>;

    auto add(int fileDescriptor, unsigned long seconds) noexcept -> void;

    auto update(int fileDescriptor, unsigned long seconds) noexcept -> void;

    auto remove(int fileDescriptor) noexcept -> void;

    auto setAwaiterOutcome(Outcome outcome) noexcept -> void;

    auto setTimingGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getTimingSubmission() noexcept -> Submission;

    [[nodiscard]] auto timing() const noexcept -> const Awaiter &;

    auto resumeTiming() const -> void;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCancelSubmission() const noexcept -> Submission;

    [[nodiscard]] auto cancel() const noexcept -> const Awaiter &;

    auto resumeCancel() const -> void;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCloseSubmission() const noexcept -> Submission;

    [[nodiscard]] auto close() const noexcept -> const Awaiter &;

    auto resumeClose() const -> void;

private:
    [[nodiscard]] static auto
    createFileDescriptor(std::source_location sourceLocation = std::source_location::current()) noexcept -> int;

    static auto setTime(int fileDescriptor,
                        std::source_location sourceLocation = std::source_location::current()) noexcept -> void;

    const int fileDescriptorIndex;
    unsigned long now, expireCount;
    std::array<std::unordered_set<int>, 601> wheel;
    std::unordered_map<int, unsigned long> location;
    Generator timingGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
