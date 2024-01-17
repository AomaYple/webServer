#include "logger.hpp"

#include "Exception.hpp"
#include "SafeQueue.hpp"

#include <filesystem>
#include <fstream>

static constinit SafeQueue<Log> logQueue;
static std::ofstream logFile;
static constinit std::atomic_flag notice;
static std::jthread worker;

auto logger::start(std::source_location sourceLocation) -> void {
    logFile.open(std::filesystem::current_path().string() + "/log.log", std::ios::app);
    if (!logFile) throw Exception{Log{Log::Level::fatal, "failed to open log file", sourceLocation}};

    worker = std::jthread([](std::stop_token stopToken) {
        while (!stopToken.stop_requested()) {
            notice.wait(false, std::memory_order::relaxed);
            notice.clear(std::memory_order::relaxed);

            for (const Log &log: logQueue.popAll()) logFile << log.toString();
        }

        logQueue.clear();
        logFile.close();
        notice.clear(std::memory_order::relaxed);
    });
}

auto logger::stop() noexcept -> void {
    worker.request_stop();

    notice.test_and_set(std::memory_order::relaxed);
    notice.notify_one();
}

auto logger::push(Log &&log) -> void {
    if (!worker.get_stop_token().stop_requested()) {
        logQueue.push(std::move(log));

        notice.test_and_set(std::memory_order::relaxed);
        notice.notify_one();
    }
}
