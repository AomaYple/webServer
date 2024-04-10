#include "Timer.hpp"

#include "../log/Exception.hpp"
#include "../ring/Submission.hpp"

#include <liburing.h>
#include <sys/timerfd.h>

#include <cstring>

auto Timer::create() -> int {
    const int fileDescriptor{Timer::createTimerFileDescriptor()};
    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

Timer::Timer(int fileDescriptor) noexcept : FileDescriptor{fileDescriptor} {}

auto Timer::timing() noexcept -> Awaiter & {
    this->getAwaiter().submit(std::make_shared<Submission>(
            this->getFileDescriptor(), Submission::Read{std::as_writable_bytes(std::span{&this->timeout, 1}), 0},
            IOSQE_FIXED_FILE));

    return this->getAwaiter();
}

auto Timer::add(int fileDescriptor, unsigned long seconds) -> void {
    const unsigned long level{seconds / (this->wheel.size() - 1)}, point{seconds % (this->wheel.size() - 1)};

    this->wheel[point].emplace(fileDescriptor, level);
    this->location.emplace(fileDescriptor, point);
}

auto Timer::update(int fileDescriptor, unsigned long seconds) -> void {
    const unsigned long level{seconds / (this->wheel.size() - 1)}, point{seconds % (this->wheel.size() - 1)};

    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->wheel[point].emplace(fileDescriptor, level);

    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(int fileDescriptor) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::clearTimeout() -> std::vector<int> {
    std::vector<int> result;

    while (this->timeout > 0) {
        {
            std::unordered_map<int, unsigned long> &wheelPoint{this->wheel[this->now]};
            for (auto element{wheelPoint.cbegin()}; element != wheelPoint.cend();) {
                if (element->second == 0) {
                    result.emplace_back(element->first);
                    this->location.erase(element->first);

                    element = wheelPoint.erase(element);
                } else
                    ++element;
            }
        }

        ++this->now;
        this->now %= this->wheel.size();
        if (this->now == 0)
            for (auto &wheelPoint: this->wheel)
                for (auto &element: wheelPoint) --element.second;

        --this->timeout;
    }

    return result;
}

auto Timer::createTimerFileDescriptor(std::source_location sourceLocation) -> int {
    const int fileDescriptor{timerfd_create(CLOCK_MONOTONIC, 0)};
    if (fileDescriptor == -1) throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    return fileDescriptor;
}

auto Timer::setTime(int fileDescriptor, std::source_location sourceLocation) -> void {
    static constexpr itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(fileDescriptor, 0, &time, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}
