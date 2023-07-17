#pragma once

enum class Type { ACCEPT, TIMEOUT, RECEIVE, SEND, CANCEL, CLOSE };

struct UserData {
    UserData(Type type, int fileDescriptor) noexcept;

    Type type;
    int fileDescriptor;
};
