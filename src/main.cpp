#include "event/EventLoop.h"

#include <thread>

using std::jthread, std::vector;

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
