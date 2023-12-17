#pragma once

#include <source_location>
#include <thread>

class Log {
public:
    enum class Level : unsigned char { info, warn, error, fatal };

    explicit Log(Level level = Level::info, std::string &&text = "",
                 std::source_location sourceLocation = std::source_location::current(),
                 std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now(),
                 std::jthread::id joinThreadId = std::this_thread::get_id()) noexcept;

    [[nodiscard]] auto toString() const -> std::string;

private:
    Level level;
    std::chrono::system_clock::time_point timestamp;
    std::jthread::id joinThreadId;
    std::source_location sourceLocation;
    std::string text;
};
