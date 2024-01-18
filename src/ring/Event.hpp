#pragma once

struct Event {
    enum class Type : unsigned char { accept, read, receive, send, close };

    Type type;
    int fileDescriptor;
};
