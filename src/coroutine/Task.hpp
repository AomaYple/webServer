#pragma once

#include "../ring/Outcome.hpp"

#include <coroutine>
#include <memory>

class Submission;

class Task {
public:
    class promise_type {
    public:
        [[nodiscard]] auto get_return_object() -> Task;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept { return std::suspend_always{}; }

        [[nodiscard]] constexpr auto final_suspend() const noexcept { return std::suspend_always{}; }

        auto unhandled_exception() const -> void;

        auto setSubmission(std::shared_ptr<Submission> newSubmission) noexcept -> void;

        [[nodiscard]] auto getSubmission() const noexcept -> const std::shared_ptr<Submission> &;

        auto setOutcome(Outcome newOutcome) noexcept -> void;

        [[nodiscard]] auto getOutcome() const noexcept -> Outcome;

    private:
        std::shared_ptr<Submission> submission;
        Outcome outcome;
    };

    explicit Task(std::coroutine_handle<promise_type> handle) noexcept;

    Task(const Task &) = delete;

    auto operator=(const Task &) = delete;

    Task(Task &&) noexcept;

    auto operator=(Task &&) noexcept -> Task &;

    ~Task();

    [[nodiscard]] auto getSubmission() const -> const std::shared_ptr<Submission> &;

    auto resume(Outcome outcome) -> void;

private:
    auto destroy() const -> void;

    std::coroutine_handle<promise_type> handle;
};
