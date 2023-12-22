#pragma once

struct Event {
    enum class Type : unsigned char { accept, timing, receive, send, close };

    Type type;
    int fileDescriptor;
};
