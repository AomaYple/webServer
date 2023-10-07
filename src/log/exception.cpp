#include "exception.hpp"

exception::exception(class log &&log) noexcept : log{std::move(log)} {}

auto exception::what() const noexcept -> const char * {
    this->message = this->log.to_string();
}
