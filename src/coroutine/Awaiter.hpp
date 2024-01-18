#pragma once

#include "../ring/Outcome.hpp"

#include <coroutine>

class Awaiter {
public:
    [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_suspend([[maybe_unused]] std::coroutine_handle<> handle) const noexcept -> void {}

    [[nodiscard]] auto await_resume() const noexcept -> Outcome;

    auto setOutcome(Outcome newOutcome) noexcept -> void;

private:
    Outcome outcome;
};
