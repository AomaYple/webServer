#include "Event.h"

Event::Event(Type type, int socket) noexcept : type{type}, socket{socket} {}
