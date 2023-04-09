#include "ThreadPool.h"
#include "Log.h"

#include <filesystem>

#include <sys/sysinfo.h>

ThreadPool::ThreadPool(unsigned short port, bool stopLog, bool writeFile) {
    int threadNumber {get_nprocs() - 2};

    if (writeFile)
        Log::writeToFile();

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
