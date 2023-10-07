#pragma once

#include <source_location>
#include <thread>

class log {
public:
    enum class level : unsigned char { info, warn, error, fatal };

    log(level level, std::string &&text, std::source_location source_location = std::source_location::current(),
        std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now(),
        std::jthread::id join_thread_id = std::this_thread::get_id()) noexcept;

    [[nodiscard]] auto to_string() const -> std::string;

private:
    level level;
    std::chrono::system_clock::time_point timestamp;
    std::jthread::id join_thread_id;
    std::source_location source_location;
    std::string text;
};
