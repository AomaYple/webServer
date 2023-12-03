#include "Log.hpp"

#include <chrono>

Log::Log(Level level, std::source_location sourceLocation, std::string &&text) noexcept
    : level{level}, sourceLocation{sourceLocation}, text{std::move(text)} {}

auto Log::toString() const noexcept -> std::string {
    constexpr std::array<const std::string_view, 4> levels{"info", "warn", "error", "fatal"};

    std::ostringstream joinThreadIdStream;
    joinThreadIdStream << this->joinThreadId;

    return std::format("{} {} {} {}:{}:{}:{} {}\n", levels[static_cast<unsigned char>(this->level)], this->timestamp,
                       joinThreadIdStream.str(), this->sourceLocation.file_name(), this->sourceLocation.line(),
                       this->sourceLocation.column(), this->sourceLocation.function_name(), this->text);
}
