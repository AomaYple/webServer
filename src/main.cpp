#include "scheduler/Scheduler.hpp"

#include <thread>

auto main() -> int {
    std::vector<std::jthread> works(std::jthread::hardware_concurrency() - 1);

    for (std::jthread &work: works)
        work = std::jthread([] {
            Scheduler scheduler;
            scheduler.run();
        });

    Scheduler scheduler;
    scheduler.run();

    return 0;
}
