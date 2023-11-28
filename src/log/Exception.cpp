#include "Exception.hpp"

Exception::Exception(Log &&log) noexcept : message{log.toString()}, log{std::move(log)} {}

auto Exception::what() const noexcept -> const char * { return this->message.c_str(); }

auto Exception::getLog() noexcept -> Log { return std::move(this->log); }
