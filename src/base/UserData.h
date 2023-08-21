#pragma once

enum class EventType { Accept, Timeout, Receive, Send, Cancel, Close };

struct UserData {
    UserData(EventType eventType, unsigned int fileDescriptor) noexcept;

    const EventType type;
    const unsigned int fileDescriptor;
};
