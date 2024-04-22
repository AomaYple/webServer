#include "Log.hpp"

#include <chrono>

Log::Log(Log::Level level, std::string &&text, std::source_location sourceLocation,
         std::chrono::system_clock::time_point timestamp, std::jthread::id joinThreadId) noexcept :
    level{level}, text{std::move(text)}, sourceLocation{sourceLocation}, timestamp{timestamp},
    joinThreadId{joinThreadId} {}

auto Log::toString() const -> std::string {
    static constexpr std::array<const std::string_view, 4> levels{"info", "warn", "error", "fatal"};

    std::ostringstream stream;
    stream << this->joinThreadId;

    return std::format("{} {} {} {}:{}:{}:{} {}\n", levels[static_cast<unsigned char>(this->level)], this->timestamp,
                       stream.str(), this->sourceLocation.file_name(), this->sourceLocation.line(),
                       this->sourceLocation.column(), this->sourceLocation.function_name(), this->text);
}

auto Log::toBytes() const -> std::vector<std::byte> {
    const std::string stringLog{this->toString()};
    const std::span<const std::byte> spanLog{std::as_bytes(std::span{stringLog})};

    return std::vector<std::byte>{spanLog.cbegin(), spanLog.cend()};
}
