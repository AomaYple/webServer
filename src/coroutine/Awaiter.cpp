#include "Awaiter.h"

using namespace std;

auto Awaiter::await_resume() const noexcept -> pair<int32_t, uint32_t> { return this->result; }

auto Awaiter::setResult(pair<int32_t, uint32_t> newResult) noexcept -> void { this->result = newResult; }
