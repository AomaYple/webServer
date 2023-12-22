#pragma once

#include "Event.hpp"

#include <sys/socket.h>

#include <span>
#include <variant>

struct Submission {
    struct AcceptParameters {
        sockaddr *address;
        socklen_t *addressLength;
        int flags;
    };

    struct ReadParameters {
        std::span<std::byte> buffer;
        unsigned long offset;
    };

    struct ReceiveParameters {
        int flags;
        int ringBufferId;
    };

    struct SendParameters {
        std::span<const std::byte> buffer;
        int flags;
        unsigned int zeroCopyFlags;
    };

    struct CloseParameters {};

    Event event;
    unsigned int flags;
    std::variant<AcceptParameters, ReadParameters, ReceiveParameters, SendParameters, CloseParameters> parameters;
};
