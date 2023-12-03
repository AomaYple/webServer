#pragma once

struct Event {
    enum class Type : unsigned char { accept, timeout, receive, send, cancel, close };

    Type type;
    int fileDescriptor;
};
