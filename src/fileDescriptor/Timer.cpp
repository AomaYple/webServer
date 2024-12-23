#include "Timer.hpp"

#include "../log/Exception.hpp"

#include <linux/io_uring.h>
#include <sys/timerfd.h>

[[nodiscard]] constexpr auto
    createTimerFileDescriptor(const std::source_location sourceLocation = std::source_location::current()) -> int {
    const int fileDescriptor{timerfd_create(CLOCK_MONOTONIC, 0)};
    if (fileDescriptor == -1) {
        throw Exception{
            Log{Log::Level::fatal, std::error_code{errno, std::generic_category()}.message(), sourceLocation}
        };
    }

    return fileDescriptor;
}

constexpr auto setTime(const int fileDescriptor,
                       const std::source_location sourceLocation = std::source_location::current()) -> void {
    constexpr itimerspec time{
        {1, 0},
        {1, 0}
    };
    if (timerfd_settime(fileDescriptor, 0, &time, nullptr) == -1) {
        throw Exception{
            Log{Log::Level::fatal, std::error_code{errno, std::generic_category()}.message(), sourceLocation}
        };
    }
}

auto Timer::create() -> int {
    const int fileDescriptor{createTimerFileDescriptor()};
    setTime(fileDescriptor);

    return fileDescriptor;
}

Timer::Timer(const int fileDescriptor) noexcept : FileDescriptor{fileDescriptor} {}

auto Timer::timing() noexcept -> Awaiter {
    return Awaiter{
        Submission{
                   this->getFileDescriptor(),
                   IOSQE_FIXED_FILE, 0,
                   0, Submission::Read{std::as_writable_bytes(std::span{&this->timeout, 1}), 0},
                   }
    };
}

auto Timer::add(const int fileDescriptor, const std::chrono::seconds seconds) -> void {
    const auto point{seconds % (this->wheel.size() - 1)};

    this->wheel[point.count()].emplace(fileDescriptor, (seconds / (this->wheel.size() - 1)).count());
    this->location.emplace(fileDescriptor, point);
}

auto Timer::update(const int fileDescriptor, const std::chrono::seconds seconds) -> void {
    const auto point{seconds % (this->wheel.size() - 1)};

    this->wheel[this->location.at(fileDescriptor).count()].erase(fileDescriptor);
    this->wheel[point.count()].emplace(fileDescriptor, (seconds / (this->wheel.size() - 1)).count());

    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(const int fileDescriptor) -> void {
    if (const auto result{this->location.find(fileDescriptor)}; result != this->location.cend()) {
        this->wheel[result->second.count()].erase(fileDescriptor);
        this->location.erase(result);
    }
}

auto Timer::clearTimeout() -> std::vector<int> {
    std::vector<int> result;

    while (this->timeout > 0) {
        std::unordered_map<int, unsigned long> &wheelPoint{this->wheel[this->now.count()]};
        for (auto element{wheelPoint.begin()}; element != wheelPoint.end();) {
            if (element->second == 0) {
                result.emplace_back(element->first);
                this->location.erase(element->first);

                element = wheelPoint.erase(element);
            } else [[likely]] {
                --element->second;
                ++element;
            }
        }

        ++this->now;
        this->now %= static_cast<decltype(this->now)>(this->wheel.size());
        --this->timeout;
    }

    return result;
}
