#include "Exception.h"

using std::source_location;
using std::string;
using std::chrono::system_clock;
using std::this_thread::get_id;

Exception::Exception(source_location sourceLocation, Level level, string &&information)
    : message{system_clock::now(), get_id(), sourceLocation, level, std::move(information)},
      messageString{this->message.combineToString()} {}

auto Exception::what() const noexcept -> const char * { return this->messageString.c_str(); }

auto Exception::getMessage() noexcept -> Message { return std::move(this->message); }
