#include "DatabaseError.h"

#include "../log/Message.h"

using std::source_location;
using std::string;
using std::chrono::system_clock;
using std::this_thread::get_id;

DatabaseError::DatabaseError(source_location sourceLocation, string &&information)
    : message{Message{system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::move(information)}
                      .combineToString()} {}

auto DatabaseError::what() const noexcept -> const char * { return message.c_str(); }
