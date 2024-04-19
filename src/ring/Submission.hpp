#pragma once

#include <span>
#include <string_view>
#include <sys/socket.h>
#include <variant>

struct Submission {
    struct Accept {
        sockaddr *address;
        socklen_t *addressLength;
        int flags;
    };

    struct Open {
        int directoryFileDescriptor;
        std::string_view path;
        int flags;
        unsigned int mode, fileDescriptorIndex;
    };

    struct Write {
        std::span<const std::byte> buffer;
        unsigned long offset;
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

    struct Cancel {
        int flags;
    };

    struct Close {};

    int fileDescriptor;
    std::variant<Accept, Open, Write, Read, Receive, Send, Cancel, Close> parameter;
    unsigned int flags;
    unsigned long userData;
};
