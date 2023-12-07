#include "logger.hpp"

#include "LogQueue.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

static constinit LogQueue logQueue;
static std::ofstream logFile;
static constinit std::atomic_flag notice;
static constinit std::mutex lock;
static std::jthread worker;

auto logger::initialize() noexcept -> void {
    logFile.open(std::filesystem::current_path().string() + "/log.log", std::ios::app);
    if (!logFile) {
        std::cout << "failed to open log file" << std::endl;

        std::terminate();
    }

    worker = std::jthread([](std::stop_token stopToken) {
        while (!stopToken.stop_requested()) {
            notice.wait(false, std::memory_order_relaxed);
            notice.clear(std::memory_order_relaxed);

            for (const Log &log: logQueue.popAll()) logFile << log.toString();
        }

        logQueue.clear();
        logFile.close();
        notice.clear(std::memory_order_relaxed);
    });
}

auto logger::destroy() noexcept -> void {
    worker.request_stop();

    notice.test_and_set(std::memory_order_relaxed);
    notice.notify_one();
}

auto logger::push(Log &&log) noexcept -> void {
    if (!worker.get_stop_token().stop_requested()) {
        logQueue.push(std::move(log));

        notice.test_and_set(std::memory_order_relaxed);
        notice.notify_one();
    }
}

auto logger::flush() noexcept -> void {
    const std::lock_guard lockGuard{lock};

    for (const Log &log: logQueue.popAll()) {
        logFile << log.toString();
        logFile.flush();
    }
}
