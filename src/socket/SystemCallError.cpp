#include "SystemCallError.hpp"

SystemCallError::SystemCallError(std::string &&message) noexcept : message{std::move(message)} {}

auto SystemCallError::what() const noexcept -> const char * { return this->message.c_str(); }
