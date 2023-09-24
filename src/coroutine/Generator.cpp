#include "Generator.hpp"

#include <exception>
#include <utility>

auto Generator::promise_type::get_return_object() -> Generator {
    return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
}

auto Generator::promise_type::unhandled_exception() const -> void { std::rethrow_exception(std::current_exception()); }

Generator::Generator(std::coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Generator::Generator(Generator &&other) noexcept : handle{std::exchange(other.handle, nullptr)} {}

auto Generator::operator=(Generator &&other) noexcept -> Generator & {
    if (this != &other) {
        this->destroy();

        this->handle = std::exchange(other.handle, nullptr);
    }

    return *this;
}

Generator::~Generator() { this->destroy(); }

auto Generator::destroy() const -> void {
    if (this->handle) this->handle.destroy();
}

auto Generator::resume() const -> void { this->handle(); }
