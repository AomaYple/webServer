#pragma once

#include <cstdint>

enum class Type { ACCEPT, TIMEOUT, RECEIVE, SEND, CANCEL, CLOSE };

struct UserData {
    UserData(Type type, std::int_fast32_t fileDescriptor) noexcept;

    Type type;
    std::int_fast32_t fileDescriptor;
};
