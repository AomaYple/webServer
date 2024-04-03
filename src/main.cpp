#include "log/logger.hpp"
#include "scheduler/Scheduler.hpp"

#include <thread>

auto main() -> int {
    logger::start();
    Scheduler::registerSignal();

    std::vector<std::jthread> workers{std::jthread::hardware_concurrency() - 2};
    for (auto &worker: workers)
        worker = std::jthread{[] {
            Scheduler scheduler;
            scheduler.run();
        }};

    Scheduler scheduler;
    scheduler.run();

    logger::stop();

    return 0;
}
