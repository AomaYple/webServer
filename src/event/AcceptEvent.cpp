#include "AcceptEvent.h"

#include "../base/BufferRing.h"
#include "../log/Log.h"
#include "../network/Server.h"
#include "Timer.h"

#include <cstring>

using std::shared_ptr;
using std::source_location;
using std::string;

auto AcceptEvent::handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                         const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                         source_location sourceLocation) const -> void {
    if (result >= 0) {
        Client client{result, 60, userRing};

        client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        Log::produce(sourceLocation, Level::ERROR,
                     "server accept error: " + string{std::strerror(static_cast<int>(std::abs(result)))});

    if (!(flags & IORING_CQE_F_MORE)) server.accept();
}
