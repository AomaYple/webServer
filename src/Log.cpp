#include "Log.h"

#include <iostream>
#include <iomanip>

using std::cout, std::string, std::string_view, std::get, std::ostringstream, std::put_time, std::this_thread::get_id,
    std::mutex, std::lock_guard, std::atomic_ref, std::chrono::system_clock, std::source_location;

auto Log::add(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
    log.addLog(sourceLocation, level, data);
}

auto Log::stopWork() -> void {
    log.stopWorkLog();
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
            auto &element {this->outputLog.front()};

            ostringstream stream;
            time_t now {system_clock::to_time_t(get<0>(element))};
            stream << put_time(localtime(&now), "%F %T ") << " ";
            cout << stream.str();

            cout << get<1>(element) << " ";

            source_location sourceLocation {get<2>(element)};
            cout << sourceLocation.file_name() << ":" << sourceLocation.line() << ":" << sourceLocation.function_name() << " ";

            switch (get<3>(element)) {
                case Level::INFO:
                    cout << "INFO ";
                    break;
                case Level::ERROR:
                    cout << "ERROR ";
                    break;
            }

            cout << get<4>(element) << "\n";

            this->outputLog.pop();
        }
    }
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

Log Log::log;
