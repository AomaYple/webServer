#include "Generator.h"

#include <exception>

using namespace std;

auto Generator::promise_type::get_return_object() noexcept -> Generator {
    return Generator{coroutine_handle<promise_type>::from_promise(*this)};
}

auto Generator::promise_type::unhandled_exception() const -> void { rethrow_exception(current_exception()); }

Generator::Generator(coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Generator::Generator(Generator &&other) noexcept : handle{other.handle} { other.handle = nullptr; }

auto Generator::operator=(Generator &&other) noexcept -> Generator & {
    if (this != &other) {
        this->handle = other.handle;
        other.handle = nullptr;
    }
    return *this;
}

auto Generator::resume() const -> void { this->handle(); }

Generator::~Generator() {
    if (this->handle) this->handle.destroy();
}
