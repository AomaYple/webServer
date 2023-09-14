#include "Awaiter.hpp"

auto Awaiter::await_resume() const noexcept -> std::pair<int, unsigned int> { return this->result; }

auto Awaiter::setResult(std::pair<int, unsigned int> newResult) noexcept -> void { this->result = newResult; }
