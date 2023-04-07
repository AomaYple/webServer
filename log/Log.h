#pragma once

#include <string_view>
#include <queue>
#include <thread>
#include <source_location>

enum class Level {
    INFO,
    WARN,
    ERROR
};

class Log {
public:
    static auto add(const std::source_location &sourceLocation, const Level &level, const std::string_view &data) -> void;

    static auto stopWork() -> void;
private:
    std::queue<std::tuple<std::chrono::system_clock::time_point, std::thread::id, std::source_location, Level, std::string>>
            inputLog, outputLog;
    bool stop {false};
    std::atomic_flag lock;
    std::jthread work;

    static Log log;

    Log() = default;

    auto addLog(const std::source_location &sourceLocation, const Level &level, const std::string_view &data) -> void;

    auto stopWorkLog() -> void;
};
