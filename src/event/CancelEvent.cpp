#include "CancelEvent.h"

#include <cstring>

#include "../log/Log.h"

using std::shared_ptr;
using std::source_location;
using std::string, std::to_string;

auto CancelEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "file descriptor " + to_string(fileDescriptor) +
                         " cancel error: " + string{std::strerror(std::abs(result))});
}
