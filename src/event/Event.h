#pragma once

#include <cstdint>
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

    virtual auto handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                        const std::shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                        std::source_location sourceLocation) const -> void = 0;

    virtual ~Event() = default;
};
