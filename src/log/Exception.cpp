#include "Exception.hpp"

Exception::Exception(Log &&log) noexcept : log{std::move(log)} {}

auto Exception::what() const noexcept -> const char * {
    *this->message = this->log.toString();
    return this->message->c_str();
}

auto Exception::getLog() noexcept -> Log { return std::move(this->log); }
