#include "UserData.h"

UserData::UserData(Type type, int fileDescriptor)

        noexcept
    : type{type}, fileDescriptor{fileDescriptor} {}
