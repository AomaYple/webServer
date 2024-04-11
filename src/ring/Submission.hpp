#pragma once

#include <sys/socket.h>

#include <span>
#include <variant>

struct Submission {
    struct Accept {
        sockaddr *address;
        socklen_t *addressLength;
        int flags;
    };

    struct Read {
        std::span<std::byte> buffer;
        unsigned long offset;
    };

    struct Receive {
        std::span<std::byte> buffer;
        int flags;
        int ringBufferId;
    };

    struct Send {
        std::span<const std::byte> buffer;
        int flags;
        unsigned int zeroCopyFlags;
    };

    struct Close {
    };

    int fileDescriptor;
    std::variant<Accept, Read, Receive, Send, Close> parameter;
    unsigned int flags;
    void *userData;
};
