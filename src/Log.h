#pragma once

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

    static auto writeToFile() -> void;
private:
    std::queue<std::tuple<std::chrono::system_clock::time_point, std::thread::id, std::source_location, Level, std::string>>
            inputLog, outputLog;
    bool stop {false}, writeFile {false};
    std::mutex lock;
    std::atomic_flag notice;
    std::jthread work;

    static Log log;

    Log();

    auto addLog(const std::source_location &sourceLocation, const Level &level, const std::string_view &data) -> void;

    auto stopWorkLog() -> void;

    auto writeToFileLog() -> void;

    static auto handleTime(const std::chrono::system_clock::time_point &timePoint) -> std::string;

    static auto handleThreadId(const std::thread::id &id) -> std::string;

    static auto handleSourceLocation(const std::source_location &sourceLocation) -> std::string;

    static auto handleLogLevel(const Level &level) -> std::string;

    static auto handleLogInformation(const std::string_view &data) -> std::string;
};
