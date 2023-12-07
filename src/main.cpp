#include "log/logger.hpp"
#include "scheduler/Scheduler.hpp"

#include <thread>

auto main() noexcept -> int {
    logger::initialize();

    std::vector<std::jthread> works(std::jthread::hardware_concurrency() - 1);

    Scheduler::registerSignal();

    for (std::jthread &work: works)
        work = std::jthread([] {
            Scheduler scheduler;
            scheduler.run();
        });

    Scheduler scheduler;
    scheduler.run();

    logger::destroy();

    return 0;
}
