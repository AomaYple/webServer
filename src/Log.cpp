#include "Log.h"

#include <iostream>
#include <iomanip>
#include <format>

using std::cout, std::string, std::string_view, std::array, std::get, std::ostringstream, std::put_time, std::this_thread::get_id,
    std::mutex, std::lock_guard, std::atomic_ref, std::chrono::system_clock, std::source_location, std::format;

constexpr array<string_view, 2> levels {"INFO", "ERROR"};

auto Log::add(source_location sourceLocation, Level level, string_view data) -> void {
    if (!log.stop) {
        {
            lock_guard<mutex> lockGuard {log.lock};
            log.inputLog.emplace(system_clock::now(), get_id(), sourceLocation, level, string {data});
        }

        log.notice.test_and_set();
        log.notice.notify_one();
    }
}

auto Log::stopWork() -> void {
    atomic_ref<bool> atomicStop {log.stop};
    atomicStop = true;
}

Log::Log() : stop(false), work([this] {
    while (!this->stop) {
        this->notice.wait(false);
        this->notice.clear();

        {
            lock_guard<mutex> lockGuard {this->lock};
            this->outputLog.swap(this->inputLog);
        }

        while (!this->outputLog.empty()) {
            auto &message {this->outputLog.front()};

            ostringstream stream;
            time_t now {system_clock::to_time_t(message.time)};
            stream << put_time(localtime(&now), "%F %T ") << " " << message.threadId << " ";
            cout << stream.str();

            cout << format("{}:{}:{}:{} {} {}\n", message.sourceLocation.file_name(), message.sourceLocation.line(),
                           message.sourceLocation.column(), message.sourceLocation.function_name(), levels[static_cast<int>(message.level)],
                           message.information);

            this->outputLog.pop();
        }
    }
}) {}

Log Log::log;
