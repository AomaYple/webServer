#include "Event.h"

#include "../base/BufferRing.h"
#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../http/Http.h"
#include "../log/Log.h"
#include "../log/message.h"
#include "../socket/Server.h"
#include "Timer.h"

#include <cstring>

using std::make_unique, std::this_thread::get_id;
using std::source_location, std::string, std::unique_ptr, std::shared_ptr;
using std::chrono::system_clock;

auto Event::create(Type type) -> unique_ptr<Event> {
    unique_ptr<Event> event;

    switch (type) {
        case Type::ACCEPT:
            event = make_unique<AcceptEvent>();
            break;
        case Type::TIMEOUT:
            event = make_unique<TimeoutEvent>();
            break;
        case Type::RECEIVE:
            event = make_unique<ReceiveEvent>();
            break;
        case Type::SEND:
            event = make_unique<SendEvent>();
            break;
        case Type::CANCEL:
            event = make_unique<CancelEvent>();
            break;
        case Type::CLOSE:
            event = make_unique<CloseEvent>();
            break;
    }

    return event;
}

auto AcceptEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                         source_location sourceLocation) const -> void {
    if (result >= 0) {
        Client client{result, 60, userRing};

        client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        Log::produce(message::combine(system_clock::now(), get_id(), sourceLocation, Level::ERROR,
                                      "accept error: " + string{std::strerror(std::abs(result))}));

    if (!(flags & IORING_CQE_F_MORE)) server.accept();
}

auto TimeoutEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                          BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                          source_location sourceLocation) const -> void {
    if (result == sizeof(std::uint64_t)) {
        timer.clearTimeout();

        timer.startTiming();
    } else
        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL,
                                         "timeout error: " + string{std::strerror(std::abs(result))})};
}

auto ReceiveEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                          BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                          source_location sourceLocation) const -> void {
    if (result <= 0 && std::abs(result) != ECANCELED) {
        string error;
        Level level;
        if (result == 0) {
            error = "client closed connection";
            level = Level::INFO;
        } else {
            error = "receive error: " + string{std::strerror(std::abs(result))};
            level = Level::WARN;
        }

        Log::produce(message::combine(system_clock::now(), get_id(), sourceLocation, level, std::move(error)));
    }

    if (!timer.exist(fileDescriptor)) return;

    if (result > 0) {
        Client client{timer.pop(fileDescriptor)};

        client.writeReceivedData(bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
            client.writeUnSendData(Http::parse(client.readReceivedData(), database));

            client.send();
        }

        if (!(flags & IORING_CQE_F_MORE)) client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        timer.pop(fileDescriptor);
}

auto SendEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                       BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                       source_location sourceLocation) const -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0)
        Log::produce(message::combine(system_clock::now(), get_id(), sourceLocation, Level::ERROR,
                                      "send error: " + string{std::strerror(std::abs(result))}));

    if (!timer.exist(fileDescriptor)) return;

    if (result == 0 && flags & IORING_CQE_F_NOTIF) {
        Client client{timer.pop(fileDescriptor)};

        client.send();

        timer.add(std::move(client));
    } else if (result < 0)
        timer.pop(fileDescriptor);
}

auto CancelEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                         source_location sourceLocation) const -> void {
    Log::produce(message::combine(system_clock::now(), get_id(), sourceLocation, Level::ERROR,
                                  "cancel error: " + string{std::strerror(std::abs(result))}));
}

auto CloseEvent::handle(__s32 result, int fileDescriptor, __u32 flags, const shared_ptr<UserRing> &userRing,
                        BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                        source_location sourceLocation) const -> void {
    Log::produce(message::combine(system_clock::now(), get_id(), sourceLocation, Level::ERROR,
                                  "close error: " + string{std::strerror(std::abs(result))}));
}
