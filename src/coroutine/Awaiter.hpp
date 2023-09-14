#pragma once

#include <coroutine>
#include <cstdint>
#include <utility>

class Awaiter {
public:
    [[nodiscard]] constexpr auto await_ready() const noexcept -> bool { return false; }

    constexpr auto await_suspend(std::coroutine_handle<> handle) const noexcept -> void {}

    [[nodiscard]] auto await_resume() const noexcept -> std::pair<int, unsigned int>;

    auto setResult(std::pair<int, unsigned int> newResult) noexcept -> void;

private:
    std::pair<int, unsigned int> result;
};
