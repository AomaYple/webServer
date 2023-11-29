#include "Awaiter.hpp"

auto Awaiter::await_resume() const noexcept -> Result { return this->result; }

auto Awaiter::setResult(Result newResult) noexcept -> void { this->result = newResult; }
