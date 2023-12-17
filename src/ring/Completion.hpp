#pragma once

#include "Event.hpp"

struct Completion {
    Event event;
    int result;
    unsigned int flags;
};
