#include "ThreadPool.h"
#include "Log.h"

#include <sys/sysinfo.h>

ThreadPool::ThreadPool(unsigned short port, bool stopLog) {
    int threadNumber {get_nprocs() - 2};

    if (stopLog) {
        Log::stopWork();
        ++threadNumber;
    }

    for (unsigned int i; i < threadNumber; ++i) {
        this->eventLoops.emplace_back(port);
        this->eventLoops.back().loop();
    }

    EventLoop eventLoop {port};
    eventLoop.loop();
}
