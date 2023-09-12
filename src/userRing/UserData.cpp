#include "UserData.h"

UserData::UserData(TaskType taskType, uint32_t fileDescriptor) noexcept
    : taskType{taskType}, fileDescriptor{fileDescriptor} {}
