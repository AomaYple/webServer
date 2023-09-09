#include "event/EventLoop.h"

#include <thread>

using namespace std;

auto main() -> int {
    vector<jthread> works(0);

    for (jthread &work: works) {
        work = jthread([] {
            EventLoop eventLoop;
            eventLoop.loop();
        });
    }

    EventLoop eventLoop;
    eventLoop.loop();

    return 0;
}
