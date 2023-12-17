#include "eventLoop/EventLoop.hpp"
#include "log/logger.hpp"

#include <thread>

auto main() -> int {
    logger::start();
    EventLoop::registerSignal();

    std::vector<std::jthread> works{std::jthread::hardware_concurrency() - 2};
    for (auto &work: works)
        work = std::jthread{[] {
            EventLoop eventLoop;
            eventLoop.run();
        }};

    EventLoop eventLoop;
    eventLoop.run();

    logger::stop();

    return 0;
}
