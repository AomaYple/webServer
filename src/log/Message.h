#pragma once

#include <source_location>
#include <thread>

enum class Level { WARN, ERROR, FATAL };

class Message {
public:
    Message(std::chrono::system_clock::time_point timestamp, std::jthread::id threadId,
            std::source_location sourceLocation, Level level, std::string &&information) noexcept;

    [[nodiscard]] auto combineToString() const -> std::string;

private:
    std::chrono::system_clock::time_point timestamp;
    std::jthread::id threadId;
    std::source_location sourceLocation;
    Level level;
    std::string information;
};
