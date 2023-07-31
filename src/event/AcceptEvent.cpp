#include "AcceptEvent.h"

#include <cstring>

#include "../base/BufferRing.h"
#include "../log/Log.h"
#include "../network/Server.h"
#include "../timer/Timer.h"

using std::shared_ptr;
using std::source_location;
using std::string;

auto AcceptEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if (result >= 0) {
        Client client{result, 30, userRing};

        client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        Log::produce(sourceLocation, Level::ERROR, "server accept error: " + string{std::strerror(std::abs(result))});

    if (!(flags & IORING_CQE_F_MORE)) server.accept();
}
