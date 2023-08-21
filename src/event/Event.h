#pragma once

#include <memory>
#include <source_location>

class BufferRing;
class Database;
class Server;
class Timer;
class UserRing;

enum class EventType;

class Event {
public:
    static auto create(EventType type) -> std::unique_ptr<Event>;

    virtual auto handle(int result, unsigned int fileDescriptor, unsigned int flags,
                        const std::shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                        Database &database, std::source_location sourceLocation) const -> void = 0;

    virtual ~Event() = default;
};

class AcceptEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class TimeoutEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class ReceiveEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class SendEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class CancelEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class CloseEvent : public Event {
public:
    auto handle(int result, unsigned int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};
