#include "message.h"

#include <chrono>

using namespace std;

constexpr array<string_view, 4> levels{"INFO", "WARN", "ERROR", "FATAL"};

auto message::combine(chrono::system_clock::time_point timestamp, jthread::id threadId, source_location sourceLocation,
                      Level level, string &&information) -> string {
    ostringstream threadIdStream;
    threadIdStream << threadId;

    return format("{} {} {}:{}:{}:{} {} {}", timestamp, threadIdStream.str(), sourceLocation.file_name(),
                  sourceLocation.line(), sourceLocation.column(), sourceLocation.function_name(),
                  levels[static_cast<std::uint_least8_t>(level)], information);
}