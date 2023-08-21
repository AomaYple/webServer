#include "UserData.h"

UserData::UserData(EventType eventType, unsigned int fileDescriptor) noexcept
    : type{eventType}, fileDescriptor{fileDescriptor} {}
