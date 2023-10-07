#include "log.hpp"

#include <chrono>

log::log(enum level level, std::string &&text, std::source_location source_location,
         std::chrono::system_clock::time_point timestamp, std::jthread::id join_thread_id) noexcept
    : level{level}, timestamp{timestamp}, join_thread_id{join_thread_id}, source_location{source_location},
      text{std::move(text)} {}

auto log::to_string() const -> std::string {
    constexpr std::array<std::string_view, 4> levels{"info", "warn", "error", "fatal"};

    std::ostringstream join_thread_id_stream;
    join_thread_id_stream << this->join_thread_id;

    return std::format("{} {} {} {}:{}:{}:{} {}\n", levels[static_cast<unsigned char>(this->level)], this->timestamp,
                       join_thread_id_stream.str(), this->source_location.file_name(), this->source_location.line(),
                       this->source_location.column(), this->source_location.function_name(), this->text);
}
