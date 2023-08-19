#pragma once

enum class Type { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(Type type, unsigned int fileDescriptor) noexcept;

    const Type type;
    const unsigned int fileDescriptor;
};
