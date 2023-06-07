#pragma once

enum class Type {
    ACCEPT, RECEIVE, SEND, CLOSE, TIME, RENEW, CANCEL
};

struct Event {
    Event(Type type, int socket);

    Type type;
    int socket;
};
