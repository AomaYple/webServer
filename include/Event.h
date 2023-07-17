#pragma once

#include <memory>
#include <source_location>

#include "UserData.h"

class UserRing;
class BufferRing;
class Server;
class Timer;
class Client;

class Event {
public:
    static auto create(Type type) -> Event *;

    virtual auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                        BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void = 0;

    virtual ~Event() = default;
};

class AcceptEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;

private:
};

class TimeoutEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};

class ReceiveEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};

class SendEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};

class CancelEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};

class CloseEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};
