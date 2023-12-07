#pragma once

#include <source_location>
#include <string_view>
#include <vector>

class Database;

namespace http {
    [[nodiscard]] auto parse(std::string_view request, Database &database,
                             std::source_location sourceLocation = std::source_location::current()) noexcept
            -> std::vector<std::byte>;
}// namespace http
