#pragma once

class Log;

namespace logger {
    auto initialize() noexcept -> void;

    auto destroy() noexcept -> void;

    auto push(Log &&log) noexcept -> void;

    auto flush() noexcept -> void;
}// namespace logger
