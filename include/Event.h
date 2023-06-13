#pragma once

enum class Type {
    ACCEPT,
    UPDATE_TIMEOUT,
    RECEIVE,
    TIMEOUT,
    REMOVE_TIMEOUT,
    CANCEL_FILE_DESCRIPTOR,
    CLOSE,
};

struct Event {
    Event(Type type, int fileDescriptor);

    Type type;
    int fileDescriptor;
};
