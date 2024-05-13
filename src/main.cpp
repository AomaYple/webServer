#include <memory>
#include <thread>
#include <vector>

import Scheduler;

auto main() -> int {
    Scheduler::registerSignal();

    std::vector<std::jthread> workers{std::jthread::hardware_concurrency() - 1};
    for (std::jthread &worker : workers) {
        worker = std::jthread{[] {
            Scheduler scheduler;
            scheduler.run();
        }};
    }

    Scheduler scheduler;
    scheduler.run();

    return 0;
}
