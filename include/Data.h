#pragma once

#include <source_location>
#include <thread>

enum class Level { INFO, WARN, ERROR };

struct Data {
    Data(std::chrono::system_clock::time_point timestamp, std::jthread::id threadId,
         std::source_location sourceLocation, Level level, std::string &&information) noexcept;

    Data(const Data &other) noexcept = default;

    Data(Data &&other) noexcept;

    auto operator=(Data &&other) noexcept -> Data &;

    std::chrono::system_clock::time_point timestamp;
    std::jthread::id threadId;
    std::source_location sourceLocation;
    Level level;
    std::string information;
};