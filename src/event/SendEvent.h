#pragma once

#include "Event.h"

class SendEvent : public Event {
public:
    auto handle(std::int_fast32_t result, std::int_fast32_t fileDescriptor, std::uint_fast32_t flags,
                const std::shared_ptr<UserRing> &userRing, BufferRing &bufferRing, Server &server, Timer &timer,
                std::source_location sourceLocation) const -> void override;
};
