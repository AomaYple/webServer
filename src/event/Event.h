#pragma once

#include <asm-generic/int-ll64.h>
#include <memory>
#include <source_location>

class BufferRing;
class Client;
class Database;
class Server;
class Timer;
class UserRing;

enum class Type;

class Event {
public:
    static auto create(Type type) -> std::unique_ptr<Event>;

    virtual auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                        BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                        std::source_location sourceLocation) const -> void = 0;

    virtual ~Event() = default;
};

class AcceptEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class TimeoutEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class ReceiveEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class SendEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class CancelEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};

class CloseEvent : public Event {
public:
    auto handle(__s32 result, int fileDescriptor, __u32 flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, Database &database,
                std::source_location sourceLocation) const -> void override;
};
