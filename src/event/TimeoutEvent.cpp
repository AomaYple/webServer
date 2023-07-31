#include "TimeoutEvent.h"

#include <cstring>

#include "../exception/Exception.h"
#include "../timer/Timer.h"

using std::shared_ptr;
using std::source_location;

auto TimeoutEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                          BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if (result == sizeof(unsigned long)) {
        timer.clearTimeout();

        timer.start(userRing->getSqe());
    } else
        throw Exception{sourceLocation, Level::FATAL, std::strerror(std::abs(result))};
}
