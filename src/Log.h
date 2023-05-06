#pragma once

#include <queue>
#include <thread>
#include <source_location>

enum class Level {
    INFO,
    ERROR
};

class Log {
public:
    static auto add(const std::source_location &sourceLocation, const Level &level, const std::string_view &data) -> void;

    static auto stopWork() -> void;

    Log(const Log &log) = delete;

    Log(Log &&log) = delete;
private:
    Log();

    auto addLog(const std::source_location &sourceLocation, const Level &level, const std::string_view &data) -> void;

    auto stopWorkLog() -> void;

    static Log log;

    std::queue<std::tuple<std::chrono::system_clock::time_point, std::jthread::id, std::source_location, Level, std::string>>
            inputLog, outputLog;
    bool stop;
    std::mutex lock;
    std::atomic_flag notice;
    std::jthread work;
};
