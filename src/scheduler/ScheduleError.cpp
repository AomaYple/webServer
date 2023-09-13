#include "ScheduleError.h"

using namespace std;

ScheduleError::ScheduleError(string &&message) noexcept : message{std::move(message)} {}

auto ScheduleError::what() const noexcept -> const char * { return this->message.c_str(); }
