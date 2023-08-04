#include "CloseEvent.h"

#include "../log/Log.h"

#include <cstring>

using std::shared_ptr;
using std::source_location;
using std::string, std::to_string;

auto CloseEvent::handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                        const std::shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                        std::source_location sourceLocation) const -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "file descriptor " + to_string(fileDescriptor) +
                         " close error: " + string{std::strerror(static_cast<int>(std::abs(result)))});
}
