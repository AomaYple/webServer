#include "UserData.h"

UserData::UserData(Type type, unsigned int fileDescriptor) noexcept : type{type}, fileDescriptor{fileDescriptor} {}
