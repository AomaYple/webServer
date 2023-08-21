#include "Event.h"

#include "../base/BufferRing.h"
#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../http/Http.h"
#include "../log/Log.h"
#include "../network/Server.h"
#include "../network/Timer.h"

#include <cstring>

using namespace std;

auto Event::create(EventType type) -> unique_ptr<Event> {
    unique_ptr<Event> event;

    switch (type) {
        case EventType::Accept:
            event = make_unique<AcceptEvent>();
            break;
        case EventType::Timeout:
            event = make_unique<TimeoutEvent>();
            break;
        case EventType::Receive:
            event = make_unique<ReceiveEvent>();
            break;
        case EventType::Send:
            event = make_unique<SendEvent>();
            break;
        case EventType::Cancel:
            event = make_unique<CancelEvent>();
            break;
        case EventType::Close:
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
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Error,
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
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, "timeout error: " + string{std::strerror(std::abs(result))})};
}

auto ReceiveEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                          const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                          Database &database, source_location sourceLocation) const -> void {
    if (result <= 0) {
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Warn,
                                  "receive error: " + string{std::strerror(std::abs(result))}));

        if (std::abs(result) == ECANCELED) return;
    }

    if (result > 0) {
        Client &client{timer.get(fileDescriptor)};

        if (!(flags & IORING_CQE_F_MORE)) client.receive(bufferRing.getId());

        client.writeReceivedData(bufferRing.getData(flags >> IORING_CQE_BUFFER_SHIFT, result));

        if (!(flags & IORING_CQE_F_SOCK_NONEMPTY)) {
            client.send(Http::parse(client.readReceivedData(), database));

            timer.update(fileDescriptor);
        }
    } else
        timer.remove(fileDescriptor);
}

auto SendEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                       const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                       Database &database, source_location sourceLocation) const -> void {
    if ((result == 0 && !(flags & IORING_CQE_F_NOTIF)) || result < 0) {
        Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Warn,
                                  "send error: " + string{std::strerror(std::abs(result))}));

        if (std::abs(result) == ECANCELED) return;

        timer.remove(fileDescriptor);
    }
}

auto CancelEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                         const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                         Database &database, source_location sourceLocation) const -> void {
    Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Warn,
                              "cancel error: " + string{std::strerror(std::abs(result))}));
}

auto CloseEvent::handle(int result, unsigned int fileDescriptor, unsigned int flags,
                        const shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                        Database &database, source_location sourceLocation) const -> void {
    Log::produce(Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation, LogLevel::Warn,
                              "close error: " + string{std::strerror(std::abs(result))}));
}
