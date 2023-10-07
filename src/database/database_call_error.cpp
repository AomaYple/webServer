#include "database_call_error.hpp"

database_call_error::database_call_error(std::string &&message) noexcept : message{std::move(message)} {}

auto database_call_error::what() const noexcept -> const char * { return this->message.c_str(); }
