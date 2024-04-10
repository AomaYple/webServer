#include "Awaiter.hpp"

#include "../ring/Submission.hpp"

auto Awaiter::await_suspend(std::coroutine_handle<Task::promise_type> newHandle) -> void {
    this->handle = newHandle;
    *static_cast<unsigned long *>(this->submission->getUserData()) =
            std::hash<std::coroutine_handle<Task::promise_type>>{}(this->handle);
    this->handle.promise().setSubmission(this->submission);
}

auto Awaiter::await_resume() const -> Outcome { return this->handle.promise().getOutcome(); }

auto Awaiter::submit(std::shared_ptr<Submission> &&newSubmission) noexcept -> void {
    this->submission = std::move(newSubmission);
}
