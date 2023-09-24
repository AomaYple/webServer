#include "UserData.hpp"

UserData::UserData(EventType eventType, unsigned int fileDescriptor) noexcept
    : eventType{eventType}, fileDescriptor{fileDescriptor} {}
