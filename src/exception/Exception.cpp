#include "Exception.h"

using namespace std;

Exception::Exception(string &&text) noexcept : text{std::move(text)} {}

auto Exception::what() const noexcept -> const char * { return this->text.c_str(); }
