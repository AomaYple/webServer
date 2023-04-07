#include "Log.h"

using std::string_view, std::atomic_ref, std::source_location;

Log Log::log;

auto Log::add(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
    log.addLog(sourceLocation, level, data);
}

auto Log::stopWork() -> void {
    log.stopWorkLog();
}

auto Log::addLog(const source_location &sourceLocation, const Level &level, const string_view &data) -> void {
}

auto Log::stopWorkLog() -> void {
    atomic_ref<bool> atomicStop {this->stop};
    atomicStop = true;
}
