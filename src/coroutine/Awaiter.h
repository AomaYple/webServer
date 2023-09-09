#pragma once

#include <coroutine>
#include <utility>

class Awaiter {
public:
    constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_suspend(std::coroutine_handle<> handle) const noexcept -> void {}

    auto await_resume() const noexcept -> std::pair<int, unsigned int>;

    auto setResult(std::pair<int, unsigned int> newResult) noexcept -> void;

private:
    std::pair<int, unsigned int> result;
};
