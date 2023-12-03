#pragma once

#include "Event.hpp"
#include "Outcome.hpp"

struct Completion {
    Event socketEvent;
    Outcome outcome;
};
