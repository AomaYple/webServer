#pragma once

#include <cstdint>

enum class TaskType : uint8_t { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(TaskType taskType, uint32_t fileDescriptor) noexcept;

    const TaskType taskType;
    const uint32_t fileDescriptor;
};
