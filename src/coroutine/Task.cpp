#include "Task.hpp"

#include <exception>
#include <utility>

auto Task::promise_type::get_return_object() -> Task {
    return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
}

auto Task::promise_type::unhandled_exception() const -> void { std::rethrow_exception(std::current_exception()); }

Task::Task(std::coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Task::Task(Task &&other) noexcept : handle{std::exchange(other.handle, nullptr)} {}

auto Task::operator=(Task &&other) noexcept -> Task & {
    if (this != &other) {
        this->destroy();
        this->handle = std::exchange(other.handle, nullptr);
    }
    return *this;
}

auto Task::resume() const -> void { this->handle(); }

Task::~Task() { this->destroy(); }

auto Task::destroy() const -> void {
    if (this->handle) this->handle.destroy();
}
