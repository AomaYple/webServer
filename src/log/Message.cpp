#include "Message.h"

#include <chrono>

using std::array;
using std::jthread;
using std::source_location;
using std::string, std::string_view;
using std::stringstream;
using std::chrono::system_clock;

constexpr array<string_view, 3> levels{"WARN", "ERROR", "FATAL"};

Message::Message(system_clock::time_point timestamp, jthread::id threadId, source_location sourceLocation, Level level,
                 string &&information) noexcept
    : timestamp{timestamp}, threadId{threadId}, sourceLocation{sourceLocation}, level{level},
      information{std::move(information)} {}

auto Message::combineToString() const -> string {
    stringstream threadIdStream;
    threadIdStream << this->threadId;

    return format("{} {} {}:{}:{}:{} {} {}\n", this->timestamp, threadIdStream.str(), this->sourceLocation.file_name(),
                  this->sourceLocation.line(), this->sourceLocation.column(), this->sourceLocation.function_name(),
                  levels[static_cast<int>(this->level)], this->information);
}
