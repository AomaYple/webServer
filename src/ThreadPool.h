#pragma once

#include "EventLoop.h"

#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(unsigned short port, bool stopLog = false, bool writeFile = false);
private:
    std::vector<EventLoop> eventLoops;
};
