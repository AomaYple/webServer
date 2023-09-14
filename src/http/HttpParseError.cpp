#include "HttpParseError.hpp"

HttpParseError::HttpParseError(std::string &&message) noexcept : message{std::move(message)} {}

auto HttpParseError::what() const noexcept -> const char * { return this->message.c_str(); }
