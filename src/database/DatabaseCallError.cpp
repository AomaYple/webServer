#include "DatabaseCallError.h"

#include "../log/Message.h"

using std::source_location;
using std::string;
using std::chrono::system_clock;
using std::this_thread::get_id;

DatabaseCallError::DatabaseCallError(source_location sourceLocation, string &&information)
    : messageString{Message{system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::move(information)}
                            .combineToString()} {}

auto DatabaseCallError::what() const noexcept -> const char * { return this->messageString.c_str(); }
