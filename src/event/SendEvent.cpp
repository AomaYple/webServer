#include "SendEvent.h"

#include <cstring>

#include "../log/Log.h"
#include "../timer/Timer.h"

using std::shared_ptr;
using std::source_location;
using std::string;

auto SendEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                       BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0)
        Log::produce(sourceLocation, Level::WARN, "client send error: " + string{std::strerror(std::abs(result))});

    if (!timer.exist(fileDescriptor)) return;

    if (result > 0 || (result == 0 && flags & IORING_CQE_F_NOTIF)) {
        Client client{timer.pop(fileDescriptor)};

        timer.add(std::move(client));
    } else
        timer.pop(fileDescriptor);
}
