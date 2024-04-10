#pragma once

#include "Outcome.hpp"

struct Completion {
    Outcome outcome;
    void *userData;
};
