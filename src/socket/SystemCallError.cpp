#include "SystemCallError.h"

using namespace std;

SystemCallError::SystemCallError(string &&message) noexcept : message{std::move(message)} {}

auto SystemCallError::what() const noexcept -> const char * { return this->message.c_str(); }
