#pragma once

#include <coroutine>

class Generator {
public:
    struct promise_type {
        [[nodiscard]] auto get_return_object() noexcept -> Generator;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

        [[nodiscard]] constexpr auto final_suspend() const noexcept -> std::suspend_always { return {}; }

        auto unhandled_exception() const -> void;
    };

    explicit Generator(std::coroutine_handle<promise_type> handle) noexcept;

    Generator(const Generator &) = delete;

    Generator(Generator &&) noexcept;

    auto operator=(Generator &&) noexcept -> Generator &;

    auto resume() const -> void;

    ~Generator();

private:
    std::coroutine_handle<promise_type> handle;
};
