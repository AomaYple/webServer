#pragma once

#include <memory>
#include <source_location>

class BufferRing;
class Client;
class Server;
class Timer;
class UserRing;

enum class Type;

class Event {
public:
    static auto create(Type type) -> std::unique_ptr<Event>;

    virtual auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                        BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void = 0;

    virtual ~Event() = default;
};
