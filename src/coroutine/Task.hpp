#pragma once

#include "../ring/Outcome.hpp"
#include "../ring/Submission.hpp"

#include <coroutine>

class Task {
public:
    class promise_type {
    public:
        [[nodiscard]] auto get_return_object() -> Task;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept { return std::suspend_always{}; }

        [[nodiscard]] constexpr auto final_suspend() const noexcept { return std::suspend_always{}; }

        auto unhandled_exception() const -> void;

        auto setSubmission(const Submission &newSubmission) noexcept -> void;

        [[nodiscard]] auto getSubmission() const noexcept -> const Submission &;

        auto setOutcome(Outcome newOutcome) noexcept -> void;

        [[nodiscard]] auto getOutcome() const noexcept -> Outcome;

    private:
        Submission submission;
        Outcome outcome;
    };

    explicit Task(std::coroutine_handle<promise_type> handle = nullptr) noexcept;

    Task(const Task &) = delete;

    auto operator=(const Task &) = delete;

    Task(Task &&) noexcept;

    auto operator=(Task &&) noexcept -> Task &;

    ~Task();

    constexpr explicit operator bool() const noexcept { return static_cast<bool>(this->handle); }

    [[nodiscard]] auto getSubmission() const -> const Submission &;

    auto resume(Outcome outcome) -> void;

    [[nodiscard]] auto done() const noexcept -> bool;

private:
    auto destroy() const -> void;

    std::coroutine_handle<promise_type> handle;
};
