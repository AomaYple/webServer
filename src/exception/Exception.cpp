#include "Exception.h"

using namespace std;

Exception::Exception(string_view text) noexcept : text{text} {}

auto Exception::what() const noexcept -> const char * { return this->text.c_str(); }
