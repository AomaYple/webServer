#pragma once

enum class Type { ACCEPT, RECEIVE, SEND, CLOSE };

struct Event {
    Event(Type type, int socket);

    Type type;
    int socket;
};
