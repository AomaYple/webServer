#include "Event.h"

#include <cstring>

#include "BufferRing.h"
#include "Http.h"
#include "Log.h"
#include "Server.h"
#include "Timer.h"

using std::runtime_error;
using std::shared_ptr;
using std::source_location;
using std::string, std::to_string;

auto Event::create(Type type) -> Event * {
    switch (type) {
        case Type::ACCEPT:
            return new AcceptEvent;
        case Type::TIMEOUT:
            return new TimeoutEvent;
        case Type::RECEIVE:
            return new ReceiveEvent;
        case Type::SEND:
            return new SendEvent;
        case Type::CANCEL:
            return new CancelEvent;
        case Type::CLOSE:
            return new CloseEvent;
    }

    return nullptr;
}

auto AcceptEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if (result >= 0) {
        Client client{result, 30, userRing};

        client.receive(bufferRing.getId());

        timer.add(std::move(client));
    } else
        Log::produce(sourceLocation, Level::WARN, "server accept error: " + string{std::strerror(std::abs(result))});

    if (!(flags & IORING_CQE_F_MORE)) server.accept();
}

auto TimeoutEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                          BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    if (result == sizeof(unsigned long)) {
        timer.clearTimeout();

        timer.start(userRing->getSqe());
    } else
        throw runtime_error("timer timing error: " + string{std::strerror(std::abs(result))});
}

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

auto CancelEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                         BufferRing &bufferRing, Server &server, Timer &timer, source_location sourceLocation) const
        -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "file descriptor " + to_string(fileDescriptor) +
                         " cancel error: " + string{std::strerror(std::abs(result))});
}
auto CloseEvent::handle(int result, int fileDescriptor, unsigned int flags, const shared_ptr<UserRing> &userRing,
                        BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
        -> void {
    Log::produce(sourceLocation, Level::WARN,
                 "file descriptor " + to_string(fileDescriptor) +
                         " close error: " + string{std::strerror(std::abs(result))});
}
