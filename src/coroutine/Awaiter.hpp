#pragma once

#include "Result.hpp"

#include <coroutine>

class Awaiter {
public:
    [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_suspend([[maybe_unused]] std::coroutine_handle<> handle) const noexcept -> void {}

    [[nodiscard]] auto await_resume() const noexcept -> Result;

    auto setResult(Result newResult) noexcept -> void;

private:
    Result result;
};
