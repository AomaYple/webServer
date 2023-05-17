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
    static auto add(std::source_location sourceLocation, Level level, std::string_view data) -> void;

    static auto stopWork() -> void;

    Log(const Log &log) = delete;

    Log(Log &&log) = delete;
private:
    Log();

    struct Message {
        std::chrono::system_clock::time_point time;
        std::jthread::id threadId;
        std::source_location sourceLocation;
        Level level;
        std::string information;
    };

    static Log log;

    std::queue<Message> inputLog, outputLog;
    bool stop;
    std::mutex lock;
    std::atomic_flag notice;
    std::jthread work;
};
