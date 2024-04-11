#include "coroutine/Scheduler.hpp"
#include "log/logger.hpp"

#include <thread>

auto main() -> int {
    logger::start();
    Scheduler::registerSignal();

    std::vector<std::jthread> workers{0};
    for (auto &worker : workers) {
        worker = std::jthread{[] {
            Scheduler scheduler;
            scheduler.run();
        }};
    }

    Scheduler scheduler;
    scheduler.run();

    logger::stop();

    return 0;
}
