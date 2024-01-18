#include "log/logger.hpp"
#include "scheduler/Scheduler.hpp"

#include <thread>

auto main() -> int {
    logger::start();
    Scheduler::registerSignal();

    std::vector<std::jthread> works{std::jthread::hardware_concurrency() - 2};
    for (auto &work: works)
        work = std::jthread{[] {
            Scheduler eventLoop;
            eventLoop.run();
        }};

    Scheduler eventLoop;
    eventLoop.run();

    logger::stop();

    return 0;
}
