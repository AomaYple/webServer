#include "event/EventLoop.h"

#include <thread>

using namespace std;

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
