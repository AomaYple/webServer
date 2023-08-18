#pragma once

enum class Type { ACCEPT, TIMEOUT, RECEIVE, SEND, CANCEL, CLOSE };

struct UserData {
    UserData(Type type, unsigned int fileDescriptor) noexcept;

    const Type type;
    const unsigned int fileDescriptor;
};
