#include "Log.h"

#include <fstream>
#include <iomanip>

using std::string_view, std::ofstream, std::ios, std::get, std::put_time, std::this_thread::get_id, std::mutex, std::lock_guard,
    std::atomic_ref, std::chrono::system_clock, std::source_location;

Log Log::log;

auto Log::add(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
    log.addLog(sourceLocation, level, data);
}

auto Log::stopWork() -> void {
    log.stopWorkLog();
}

Log::Log() : work([this] {
    ofstream file {"log.log", ios::trunc};

    while (!this->stop) {
        this->notice.wait(false);
        this->notice.clear();

        {
            lock_guard<mutex> lockGuard {this->lock};
            this->outputLog.swap(this->inputLog);
        }

        while (!this->outputLog.empty()) {
            auto &log {this->outputLog.front()};

            time_t now {system_clock::to_time_t(get<0>(log))};
            file << put_time(localtime(&now), "%F %T ");

            file << get<1>(log) << " ";

            source_location &sourceLocation {get<2>(log)};
            file << sourceLocation.file_name() << ":" << sourceLocation.line() << ":" << sourceLocation.function_name() << " ";

            switch (get<3>(log)) {
                case Level::INFO:
                    file << "INFO ";
                    break;
                case Level::WARN:
                    file << "WARN ";
                    break;
                case Level::ERROR:
                    file << "ERROR ";
                    break;
            }

            file << get<4>(log) << "\n";

            this->outputLog.pop();
        }
    }

    file.close();
}) {}

auto Log::addLog(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
    if (!this->stop) {
        {
            lock_guard<mutex> lockGuard {this->lock};

            this->inputLog.emplace(system_clock::now(), get_id(), sourceLocation, level, data);
        }

        this->notice.test_and_set();
        this->notice.notify_one();
    }
}

auto Log::stopWorkLog() -> void {
    atomic_ref<bool> atomicStop {this->stop};
    atomicStop = true;
}
