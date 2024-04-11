#include "Awaiter.hpp"

#include <utility>

Awaiter::Awaiter(Awaiter &&other) noexcept : handle{std::exchange(other.handle, nullptr)},
                                             submission{other.submission} { other.submission.userData = nullptr; }

auto Awaiter::operator=(Awaiter &&other) noexcept -> Awaiter & {
    if (this == &other) return *this;

    delete static_cast<unsigned long *>(this->submission.userData);
    this->handle = std::exchange(other.handle, nullptr);
    this->submission = other.submission;
    other.submission.userData = nullptr;

    return *this;
}

Awaiter::~Awaiter() { delete static_cast<unsigned long *>(this->submission.userData); }

auto Awaiter::await_suspend(std::coroutine_handle<Task::promise_type> newHandle) -> void {
    this->handle = newHandle;
    *static_cast<unsigned long *>(this->submission.userData) = std::hash<std::coroutine_handle<Task::promise_type> >
            {}(this->handle);
    this->handle.promise().setSubmission(this->submission);
}

auto Awaiter::await_resume() const -> Outcome { return this->handle.promise().getOutcome(); }

auto Awaiter::submit(const Submission &newSubmission) noexcept -> void {
    this->submission = {newSubmission.fileDescriptor, newSubmission.parameter, newSubmission.flags};
    if (this->submission.userData == nullptr) this->submission.userData = new unsigned long;
}
