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

using namespace std;

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

auto AcceptEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                         const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                         Database &database, source_location sourceLocation) const -> void {
    if (result >= 0) {
        Client client{static_cast<unsigned int>(result), 60, userRing};

        client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        Log::produce(message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, Level::ERROR,
                                      "accept error: " + string{std::strerror(std::abs(result))}));

    if (!(flags & IORING_CQE_F_MORE)) server.accept();
}

auto TimeoutEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                          const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                          Database &database, source_location sourceLocation) const -> void {
    if (result == sizeof(unsigned long)) {
        timer.clearTimeout();

        timer.startTiming();
    } else
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "timeout error: " + string{std::strerror(std::abs(result))})};
}

auto ReceiveEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                          const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                          Database &database, source_location sourceLocation) const -> void {
    if (result <= 0)
        Log::produce(message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, Level::ERROR,
                                      "receive error: " + string{std::strerror(std::abs(result))}));

    if (!timer.exist(fileDescriptor)) return;

    if (result > 0) {
        Client client{timer.pop(fileDescriptor)};

        client.writeReceivedData(bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) client.send(Http::parse(client.readReceivedData(), database));

        if (!(flags & IORING_CQE_F_MORE)) client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        timer.pop(fileDescriptor);
}

auto SendEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                       const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                       Database &database, source_location sourceLocation) const -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0)
        Log::produce(message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, Level::ERROR,
                                      "send error: " + string{std::strerror(std::abs(result))}));

    if (!timer.exist(fileDescriptor)) return;

    if (result == 0 && flags & IORING_CQE_F_NOTIF) {
        Client client{timer.pop(fileDescriptor)};

        timer.add(std::move(client));
    } else if (result < 0)
        timer.pop(fileDescriptor);
}

auto CancelEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                         const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                         Database &database, source_location sourceLocation) const -> void {
    Log::produce(message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, Level::ERROR,
                                  "cancel error: " + string{std::strerror(std::abs(result))}));
}

auto CloseEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                        const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                        Database &database, source_location sourceLocation) const -> void {
    Log::produce(message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, Level::ERROR,
                                  "close error: " + string{std::strerror(std::abs(result))}));
}
