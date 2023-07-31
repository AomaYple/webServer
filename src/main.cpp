#include <thread>

#include "event/EventLoop.h"

using std::jthread;
using std::vector;

int main() {
    vector<jthread> works(jthread::hardware_concurrency() - 2);

    for (jthread &work: works) {
        work = jthread([] {
            EventLoop eventLoop;
            eventLoop.loop();
        });
    }

    EventLoop eventLoop;
    eventLoop.loop();
}