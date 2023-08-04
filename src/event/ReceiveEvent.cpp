#include "ReceiveEvent.h"

#include "../base/BufferRing.h"
#include "../http/Http.h"
#include "../log/Log.h"
#include "Timer.h"

#include <cstring>

using std::shared_ptr;
using std::source_location;
using std::string;

auto ReceiveEvent::handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                          const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                          source_location sourceLocation) const -> void {
    if (result < 0)
        Log::produce(sourceLocation, Level::WARN,
                     "client receive error: " + string{std::strerror(static_cast<int>(std::abs(result)))});

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
