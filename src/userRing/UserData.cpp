#include "UserData.h"

UserData::UserData(TaskType taskType, unsigned int fileDescriptor) noexcept
    : taskType{taskType}, fileDescriptor{fileDescriptor} {}
