#include "Generator.hpp"

#include <utility>

auto Generator::promise_type::get_return_object() -> Generator {
    return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
}

auto Generator::promise_type::unhandled_exception() const -> void { throw; }

Generator::Generator(std::coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Generator::Generator(Generator &&other) noexcept : handle{std::exchange(other.handle, nullptr)} {}

auto Generator::operator=(Generator &&other) noexcept -> Generator & {
    this->destroy();
    this->handle = std::exchange(other.handle, nullptr);

    return *this;
}

auto Generator::resume() const -> void { this->handle(); }

auto Generator::destroy() const -> void {
    if (this->handle) this->handle.destroy();
}
