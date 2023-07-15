#pragma once

enum class Type { ACCEPT, TIMEOUT, RECEIVE, SEND, CANCEL, CLOSE };

struct Event {
    Event(Type type, int socket) noexcept;

    Type type;
    int socket;
};