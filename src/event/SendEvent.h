#pragma once

#include "Event.h"

class SendEvent : public Event {
public:
    auto handle(int result, int fileDescriptor, unsigned int flags, const std::shared_ptr<UserRing> &userRing,
                BufferRing &bufferRing, Server &server, Timer &timer, std::source_location sourceLocation) const
            -> void override;
};
