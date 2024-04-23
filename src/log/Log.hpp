#pragma once

#include <source_location>
#include <thread>
#include <vector>

class Log {
public:
    enum class Level : unsigned char { info, warn, error, fatal };

    explicit Log(Level level = {}, std::string &&text = {},
                 std::source_location sourceLocation = std::source_location::current(),
                 std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now(),
                 std::jthread::id joinThreadId = std::this_thread::get_id()) noexcept;

    [[nodiscard]] auto toString() const -> std::string;

    [[nodiscard]] auto toByte() const -> std::vector<std::byte>;

private:
    Level level;
    std::string text;
    std::source_location sourceLocation;
    std::chrono::system_clock::time_point timestamp;
    std::jthread::id joinThreadId;
};
