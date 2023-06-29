#include <thread>

#include <sys/sysinfo.h>

#include "EventLoop.h"

using std::vector, std::jthread;

int main() {
    vector<jthread> works(get_nprocs() - 2);

    for (jthread &work: works) {
        work = jthread([] {
            EventLoop eventLoop;
            eventLoop.loop();
        });
    }

    EventLoop eventLoop;
    eventLoop.loop();
}
