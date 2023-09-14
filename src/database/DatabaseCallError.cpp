#include "DatabaseCallError.hpp"

DatabaseCallError::DatabaseCallError(std::string &&message) noexcept : message{std::move(message)} {}

auto DatabaseCallError::what() const noexcept -> const char * { return this->message.c_str(); }
