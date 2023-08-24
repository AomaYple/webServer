#pragma once

enum class EventType { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(EventType eventType, unsigned int fileDescriptor) noexcept;

    const EventType eventType;
    const unsigned int fileDescriptor;
};
