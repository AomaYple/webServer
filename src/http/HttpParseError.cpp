#include "HttpParseError.h"

using namespace std;

HttpParseError::HttpParseError(string &&message) noexcept : message{std::move(message)} {}

auto HttpParseError::what() const noexcept -> const char * { return this->message.c_str(); }
