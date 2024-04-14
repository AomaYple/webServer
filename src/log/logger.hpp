#pragma once

#include <source_location>

class Log;

namespace logger {
    auto start(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto stop() noexcept -> void;

    auto push(Log &&log) -> void;
}    // namespace logger
