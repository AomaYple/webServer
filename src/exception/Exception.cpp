#include "Exception.h"

using namespace std;

Exception::Exception(string &&error) : error{std::move(error)} {}

auto Exception::what() const noexcept -> const char * { return this->error.c_str(); }
