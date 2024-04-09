#pragma once

#include "Task.hpp"

class Awaiter {
public:
    [[nodiscard]] constexpr auto await_ready() const noexcept { return false; }

    auto await_suspend(std::coroutine_handle<Task::promise_type> newHandle) -> void;

    [[nodiscard]] auto await_resume() const -> Outcome;

    auto submit(const Submission &newSubmission) noexcept -> void;

private:
    std::coroutine_handle<Task::promise_type> handle;
    Submission submission;
};
