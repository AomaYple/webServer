#include "Awaiter.hpp"

auto Awaiter::await_suspend(std::coroutine_handle<Task::promise_type> newHandle) -> void {
    if (this->handle != newHandle) {
        this->handle = newHandle;
        this->submission.userData = std::hash<std::coroutine_handle<Task::promise_type>>{}(this->handle);
        this->handle.promise().setSubmission(this->submission);
    }
}

auto Awaiter::await_resume() const -> Outcome { return this->handle.promise().getOutcome(); }

auto Awaiter::submit(const Submission &newSubmission) noexcept -> void { this->submission = newSubmission; }
