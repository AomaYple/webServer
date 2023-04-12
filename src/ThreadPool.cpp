#include "ThreadPool.h"
#include "EventLoop.h"
#include "Log.h"

#include <list>

#include <sys/sysinfo.h>

using std::list;

ThreadPool::ThreadPool(unsigned short port, bool stopLog) {
    unsigned short threadNumber {static_cast<unsigned short>(get_nprocs() - 2)};

    if (stopLog) {
        Log::stopWork();
        ++threadNumber;
    }

    list<EventLoop> eventLoops;
    for (unsigned short i {0}; i < threadNumber; ++i)
        eventLoops.emplace_back(port, true);

    EventLoop eventLoop {port};
}
