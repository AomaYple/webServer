#include "Awaiter.h"

using namespace std;

auto Awaiter::await_resume() const noexcept -> pair<int, unsigned int> { return this->result; }

auto Awaiter::setResult(pair<int, unsigned int> newResult) noexcept -> void { this->result = newResult; }
