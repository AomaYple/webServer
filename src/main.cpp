#include "coroutine/Scheduler.hpp"

auto main() -> int {
    Scheduler::registerSignal();

    for (std::vector<std::jthread> workers{std::jthread::hardware_concurrency() - 1}; auto &worker : workers) {
        worker = std::jthread{[] {
            Scheduler scheduler;
            scheduler.run();
        }};
    }

    Scheduler scheduler;
    scheduler.run();

    return 0;
}
