#pragma once

struct Event {
    enum class Type : unsigned char { accept, timing, receive, send, cancel, close };

    Type type;
    int fileDescriptor;
};
