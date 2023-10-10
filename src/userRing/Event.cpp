#include "Event.hpp"

Event::Event(Type type, unsigned int fileDescriptor) noexcept : type{type}, fileDescriptor{fileDescriptor} {}
