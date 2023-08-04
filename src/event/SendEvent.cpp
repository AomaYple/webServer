#include "SendEvent.h"

#include "../log/Log.h"
#include "Timer.h"

#include <cstring>

using std::shared_ptr;
using std::source_location;
using std::string;

auto SendEvent::handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                       const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                       source_location sourceLocation) const -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0)
        Log::produce(sourceLocation, Level::WARN,
                     "client send error: " + string{std::strerror(static_cast<int>(std::abs(result)))});

    if (!timer.exist(fileDescriptor)) return;

    if (result > 0 || (result == 0 && flags & IORING_CQE_F_NOTIF)) {
        Client client{timer.pop(fileDescriptor)};

        timer.add(std::move(client));
    } else
        timer.pop(fileDescriptor);
}
