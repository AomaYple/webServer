#include "Log.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

using std::cout, std::string, std::string_view, std::stringstream, std::ofstream, std::get, std::filesystem::current_path,
    std::put_time, std::thread, std::this_thread::get_id, std::mutex, std::lock_guard, std::atomic_ref, std::chrono::system_clock,
    std::source_location;

Log Log::log;

auto Log::add(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
    log.addLog(sourceLocation, level, data);
}

auto Log::stopWork() -> void {
    log.stopWorkLog();
}

auto Log::writeToFile() -> void {
    log.writeToFileLog();
}

Log::Log() : work([this] {
    while (!this->stop) {
        ofstream file;
        if (this->writeFile)
            file.open(current_path().string() + "/log.log");

        this->notice.wait(false);
        this->notice.clear();

        {
            lock_guard<mutex> lockGuard {this->lock};
            this->outputLog.swap(this->inputLog);
        }

        while (!this->outputLog.empty()) {
            auto &element {this->outputLog.front()};

            string time {handleTime(get<0>(element))},

                threadId {handleThreadId(get<1>(element))},

                sourceLocation {handleSourceLocation(get<2>(element))},

                logLevel {handleLogLevel(get<3>(element))},

                logInformation {handleLogInformation(get<4>(element))};

            if (!this->writeFile)
                cout << time << threadId << sourceLocation << logLevel << logInformation;
            else
                file << time << threadId << sourceLocation << logLevel << logInformation;

            this->outputLog.pop();
        }

        if (this->writeFile)
            file.close();
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

auto Log::writeToFileLog() -> void {
    atomic_ref<bool> atomicWriteFile {this->writeFile};
    atomicWriteFile = true;
}

auto Log::handleTime(const system_clock::time_point &timePoint) -> string {
    stringstream stringStream;

    time_t now {system_clock::to_time_t(timePoint)};
    stringStream << put_time(localtime(&now), "%F %T ");

    return stringStream.str();
}

auto Log::handleThreadId(const thread::id &id) -> string {
    stringstream stringStream;

    stringStream << id << " ";

    return stringStream.str();
}

auto Log::handleSourceLocation(const source_location &sourceLocation) -> string {
    stringstream stringStream;

    stringStream << sourceLocation.file_name() << ":" << sourceLocation.line() << ":" << sourceLocation.function_name() << " ";

    return stringStream.str();
}

auto Log::handleLogLevel(const Level &level) -> string {
    stringstream stringStream;

    switch (level) {
        case Level::INFO:
            stringStream << "INFO ";
            break;
        case Level::WARN:
            stringStream << "WARN ";
            break;
        case Level::ERROR:
            stringStream << "ERROR ";
            break;
    }

    return stringStream.str();
}

auto Log::handleLogInformation(const string_view &data) -> string {
    stringstream stringStream;

    stringStream << data << "\n";

    return stringStream.str();
}
