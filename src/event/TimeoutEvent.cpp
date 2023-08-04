#include "TimeoutEvent.h"

#include "../exception/Exception.h"
#include "Timer.h"

#include <cstring>

using std::shared_ptr;
using std::source_location;

auto TimeoutEvent::handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                          const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                          source_location sourceLocation) const -> void {
    if (result == sizeof(unsigned long)) {
        timer.clearTimeout();

        timer.start(userRing->getSqe());
    } else
        throw Exception{sourceLocation, Level::FATAL, std::strerror(static_cast<int>(std::abs(result)))};
}
