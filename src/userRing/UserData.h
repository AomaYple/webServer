#pragma once

enum class TaskType : unsigned char { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(TaskType taskType, unsigned int fileDescriptor) noexcept;

    const TaskType taskType;
    const unsigned int fileDescriptor;
};
