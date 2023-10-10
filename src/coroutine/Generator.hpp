#pragma once

#include <coroutine>

class Generator {
public:
    struct promise_type {
        [[nodiscard]] auto get_return_object() const -> Generator;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

        [[nodiscard]] constexpr auto final_suspend() const noexcept -> std::suspend_always { return {}; }

        auto unhandled_exception() const -> void;
    };

    explicit Generator(std::coroutine_handle<const promise_type> handle = nullptr) noexcept;

    Generator(const Generator &) = delete;

    Generator(Generator &&) noexcept;

    auto operator=(const Generator &) -> Generator & = delete;

    auto operator=(Generator &&) noexcept -> Generator &;

    ~Generator();

    auto resume() const -> void;

private:
    auto destroy() const -> void;

    std::coroutine_handle<const promise_type> handle;
};
