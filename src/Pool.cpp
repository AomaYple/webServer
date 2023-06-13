#include "Pool.h"

#include <sys/sysinfo.h>

#include <vector>

#include "EventLoop.h"
#include "Log.h"

using std::vector, std::jthread;

Pool::Pool(unsigned short port, bool logWork) {
    unsigned short threadNumber{static_cast<unsigned short>(get_nprocs() - 1)};

    if (!logWork) {
        ++threadNumber;

        Log::stopWork();
    }

    vector<jthread> works;

    for (unsigned short i{0}; i < threadNumber; ++i)
        works.emplace_back([port] {
            EventLoop eventLoop{port};
            eventLoop.loop();
        });

    EventLoop eventLoop{9999};
    eventLoop.loop();
}
