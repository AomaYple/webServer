#include "ReceiveEvent.h"

#include <cstring>

#include "../base/BufferRing.h"
#include "../http/Http.h"
#include "../log/Log.h"
#include "../timer/Timer.h"

using std::shared_ptr;
using std::source_location;
using std::string;

auto ReceiveEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                          BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if (result <= 0)
        Log::produce(sourceLocation, Level::WARN, "client receive error: " + string{std::strerror(std::abs(result))});

    if (!timer.exist(fileDescriptor)) return;

    if (result > 0) {
        Client client{timer.pop(fileDescriptor)};

        client.writeReceivedData(bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) client.send(Http::parse(client.readReceivedData()));

        if (!(flags & IORING_CQE_F_MORE)) client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        timer.pop(fileDescriptor);
}
