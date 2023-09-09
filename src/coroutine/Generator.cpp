#include "Generator.h"

#include <exception>
#include <utility>

using namespace std;

auto Generator::promise_type::get_return_object() -> Generator {
    return Generator{coroutine_handle<promise_type>::from_promise(*this)};
}

auto Generator::promise_type::unhandled_exception() const -> void { rethrow_exception(current_exception()); }

Generator::Generator(coroutine_handle<promise_type> handle) noexcept : handle{handle} {}

Generator::Generator(Generator &&other) noexcept : handle{exchange(other.handle, nullptr)} {}

auto Generator::operator=(Generator &&other) noexcept -> Generator & {
    if (this != &other) this->handle = exchange(other.handle, nullptr);
    return *this;
}

auto Generator::resume() const -> void { this->handle(); }

Generator::~Generator() {
    if (this->handle) this->handle.destroy();
}
