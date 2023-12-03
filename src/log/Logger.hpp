#pragma once

#include "LogQueue.hpp"

#include <fstream>

class Logger {
public:
    Logger(const Logger &) = delete;

    Logger(Logger &&) = delete;

    auto operator=(const Logger &) -> Logger & = delete;

    auto operator=(Logger &&) -> Logger & = delete;

    ~Logger();

    static auto push(Log &&log) -> void;

    static auto stop() noexcept -> void;

private:
    Logger();

    auto run(std::stop_token stopToken) -> void;

    auto output() -> void;

    static Logger instance;

    std::ofstream logFile;
    std::atomic_flag notice;
    LogQueue logQueue;
    std::jthread worker;
};
