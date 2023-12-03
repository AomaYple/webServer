#pragma once

#include <source_location>
#include <thread>

class Log {
public:
    enum class Level : unsigned char { info, warn, error, fatal };

    Log(Level level, std::source_location sourceLocation, std::string &&text) noexcept;

    [[nodiscard]] auto toString() const noexcept -> std::string;

private:
    Level level;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::jthread::id joinThreadId{std::this_thread::get_id()};
    std::source_location sourceLocation;
    std::string text;
};
