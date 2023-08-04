#include "UserData.h"

UserData::UserData(Type type, std::int_fast32_t fileDescriptor) noexcept : type{type}, fileDescriptor{fileDescriptor} {}
