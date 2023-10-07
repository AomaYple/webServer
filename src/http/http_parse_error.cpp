#include "http_parse_error.hpp"

http_parse_error::http_parse_error(class log &&log) noexcept : message{log.to_string()}, log{std::move(log)} {}

auto http_parse_error::what() const noexcept -> const char * { return this->message.c_str(); }

auto http_parse_error::get_log() noexcept -> class log {
    return std::move(this->log);
}
