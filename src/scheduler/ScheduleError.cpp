#include "ScheduleError.hpp"

ScheduleError::ScheduleError(std::string &&message) noexcept : message{std::move(message)} {}

auto ScheduleError::what() const noexcept -> const char * { return this->message.c_str(); }
