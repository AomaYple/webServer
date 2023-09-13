#include "UserRingCallError.h"

using namespace std;

UserRingCallError::UserRingCallError(string &&message) noexcept : message{std::move(message)} {}

auto UserRingCallError::what() const noexcept -> const char * { return this->message.c_str(); }
