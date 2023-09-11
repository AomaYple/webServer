#include "Task.h"

#include <exception>
#include <utility>

using namespace std;

auto Task::promise_type::get_return_object() -> Task {
    return Task{coroutine_handle<promise_type>::from_promise(*this)};
}

auto Task::promise_type::unhandled_exception() const -> void { rethrow_exception(current_exception()); }

Task::Task(coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Task::Task(Task &&other) noexcept : handle{exchange(other.handle, nullptr)} {}

auto Task::operator=(Task &&other) noexcept -> Task & {
    if (this != &other) {
        if (this->handle) this->handle.destroy();
        this->handle = exchange(other.handle, nullptr);
    }
    return *this;
}

auto Task::resume() const -> void { this->handle(); }

Task::~Task() {
    if (this->handle) this->handle.destroy();
}
