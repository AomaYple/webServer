#pragma once

#include "Task.hpp"

class Awaiter {
public:
    [[nodiscard]] constexpr auto await_ready() const noexcept { return false; }

    auto await_suspend(std::coroutine_handle<Task::promise_type> newHandle) -> void;

    [[nodiscard]] auto await_resume() const -> Outcome;

    auto submit(std::shared_ptr<Submission> &&newSubmission) noexcept -> void;

private:
    std::coroutine_handle<Task::promise_type> handle;
    std::shared_ptr<Submission> submission;
};
