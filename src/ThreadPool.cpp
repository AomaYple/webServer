#include "ThreadPool.h"
#include "EventLoop.h"
#include "Log.h"

#include <list>

#include <sys/sysinfo.h>

using std::vector;

ThreadPool::ThreadPool(unsigned short port, bool stopLog) {
    int threadNumber {get_nprocs() - 2};

    if (stopLog) {
        Log::stopWork();
        ++threadNumber;
    }

    std::list<EventLoop> eventLoops;

    for (unsigned int i {0}; i < threadNumber; ++i)
        eventLoops.emplace_back(port, true);

    EventLoop eventLoop {port};
}
