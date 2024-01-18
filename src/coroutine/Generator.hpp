#pragma once

#include <coroutine>

class Generator {
public:
    struct promise_type {
        auto get_return_object() -> Generator;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_never {
            return std::suspend_never{};
        }

        [[nodiscard]] constexpr auto final_suspend() const noexcept -> std::suspend_always {
            return std::suspend_always{};
        }

        auto unhandled_exception() const -> void;
    };

    explicit Generator(std::coroutine_handle<promise_type> handle) noexcept;

    Generator(const Generator &) = delete;

    auto operator=(const Generator &) -> Generator & = delete;

    Generator(Generator &&) noexcept;

    auto operator=(Generator &&) noexcept -> Generator &;

    auto resume() const -> void;

private:
    auto destroy() const -> void;

    std::coroutine_handle<promise_type> handle;
};
