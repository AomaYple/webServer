#include "Awaiter.hpp"

auto Awaiter::await_resume() const noexcept -> Outcome { return this->outcome; }

auto Awaiter::setOutcome(Outcome newOutcome) noexcept -> void { this->outcome = newOutcome; }
