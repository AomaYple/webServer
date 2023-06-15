#include "Pool.h"

#include <sys/sysinfo.h>

#include "EventLoop.h"
#include "Log.h"

using std::vector, std::jthread;

Pool::Pool(unsigned short port, bool stopLog) {
    unsigned short threadNumber{static_cast<unsigned short>(get_nprocs())};

    if (stopLog) {
        ++threadNumber;

        Log::stopWork();
    }

    vector<jthread> works;
    for (unsigned short i{0}; i < threadNumber; ++i) works.emplace_back(EventLoop{port});

    EventLoop eventLoop{9999};
    eventLoop();
}
