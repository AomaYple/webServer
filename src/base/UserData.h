#pragma once

enum class EventType : unsigned char { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(EventType eventType, unsigned int fileDescriptor) noexcept;

    const EventType eventType;
    const unsigned int fileDescriptor;
};
