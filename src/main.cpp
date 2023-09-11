#include "scheduler/Scheduler.h"

#include <thread>

using namespace std;

auto main() -> int {
    vector<jthread> works(jthread::hardware_concurrency() - 1);

    for (jthread &work: works) {
        work = jthread([] {
            Scheduler scheduler;
            scheduler.run();
        });
    }

    Scheduler scheduler;
    scheduler.run();

    return 0;
}
