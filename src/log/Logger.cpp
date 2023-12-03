#include "Logger.hpp"

#include <filesystem>

Logger::~Logger() { this->output(); }

auto Logger::push(Log &&log) -> void {
    Logger::instance.logQueue.push(std::move(log));

    Logger::instance.notice.test_and_set(std::memory_order_relaxed);
    Logger::instance.notice.notify_one();
}

auto Logger::stop() noexcept -> void {
    Logger::instance.worker.request_stop();

    Logger::instance.notice.test_and_set(std::memory_order_relaxed);
    Logger::instance.notice.notify_one();
}

Logger::Logger()
    : logFile{std::filesystem::current_path().string() + "/log.log", std::ofstream::trunc},
      worker{&Logger::run, &Logger::instance} {}

auto Logger::run(std::stop_token stopToken) -> void {
    while (!stopToken.stop_requested()) {
        this->notice.wait(false, std::memory_order_relaxed);
        this->notice.clear(std::memory_order_relaxed);

        this->output();
    }
}

auto Logger::output() -> void {
    for (const Log &log: this->logQueue.popAll()) this->logFile << log.toString();
}

Logger Logger::instance;
