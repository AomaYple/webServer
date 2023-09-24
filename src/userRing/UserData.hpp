#pragma once

enum class EventType : unsigned char { Accept, Timeout, Receive, Send, Close };

struct UserData {
    UserData(EventType eventType, unsigned int fileDescriptor) noexcept;

    EventType eventType;
    unsigned int fileDescriptor;
};
