#include "HttpParseError.h"

using std::source_location;
using std::string;
using std::chrono::system_clock;
using std::this_thread::get_id;

HttpParseError::HttpParseError(source_location sourceLocation, string &&information)
    : message{system_clock::now(), get_id(), sourceLocation, Level::WARN, std::move(information)},
      messageString{this->message.combineToString()} {}

auto HttpParseError::what() const noexcept -> const char * { return this->messageString.c_str(); }

auto HttpParseError::getMessage() noexcept -> Message { return std::move(this->message); }
