#pragma once

struct Event {
    enum class Type : unsigned char { accept, timeout, receive, send, cancel, close };

    Event(Type type, unsigned int fileDescriptor) noexcept;

    Type type;
    unsigned int fileDescriptor;
};
