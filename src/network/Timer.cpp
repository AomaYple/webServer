#include "Timer.h"

#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../userRing/Submission.h"
#include "../userRing/UserData.h"

#include <sys/timerfd.h>

#include <algorithm>
#include <cstring>

using namespace std;

auto Timer::create() -> unsigned int {
    const int fileDescriptor{Timer::createFileDescriptor(CLOCK_MONOTONIC, 0)};

    const itimerspec time{{1, 0}, {1, 0}};
    Timer::setTime(fileDescriptor, 0, time, nullptr);

    return static_cast<unsigned int>(fileDescriptor);
}

Timer::Timer(unsigned int fileDescriptorIndex)
    : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0}, timingTask{nullptr}, cancelTask{nullptr},
      closeTask{nullptr} {}

Timer::Timer(Timer &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, now{other.now}, expireCount{other.expireCount},
      wheel{std::move(other.wheel)}, location{std::move(other.location)}, timingTask{std::move(other.timingTask)},
      cancelTask{std::move(other.cancelTask)}, closeTask{std::move(other.closeTask)},
      awaiter{std::move(other.awaiter)} {}

auto Timer::createFileDescriptor(__clockid_t clockId, int flags, source_location sourceLocation) -> int {
    const int fileDescriptor{timerfd_create(clockId, flags)};

    if (fileDescriptor == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};

    return fileDescriptor;
}

auto Timer::setTime(int fileDescriptor, int flags, const itimerspec &newTime, itimerspec *oldTime,
                    source_location sourceLocation) -> void {
    if (timerfd_settime(fileDescriptor, flags, &newTime, oldTime) == -1)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, strerror(errno))};
}

auto Timer::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Timer::timing(io_uring_sqe *sqe) noexcept -> const Awaiter & {
    const unsigned long long offset{0};
    const Submission submission{sqe,
                                static_cast<int>(this->fileDescriptorIndex),
                                {as_writable_bytes(span{&this->expireCount, 1})},
                                offset};

    const UserData userData{TaskType::Timeout, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::setTimingTask(Task &&task) noexcept -> void { this->timingTask = std::move(task); }

auto Timer::resumeTiming(pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->timingTask.resume();
}

auto Timer::clearTimeout() -> vector<unsigned int> {
    vector<unsigned int> timeoutFileDescriptors;

    while (this->expireCount > 0) {
        auto &fileDescriptors{this->wheel[this->now]};

        ranges::for_each(fileDescriptors, [&timeoutFileDescriptors, this](unsigned int fileDescriptor) {
            timeoutFileDescriptors.emplace_back(fileDescriptor);

            this->location.erase(fileDescriptor);
        });

        fileDescriptors.clear();

        this->now = (this->now + 1) % this->wheel.size();

        --this->expireCount;
    }

    return timeoutFileDescriptors;
}

auto Timer::add(unsigned int fileDescriptor, unsigned char timeout, source_location sourceLocation) -> void {
    if (timeout >= this->wheel.size())
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "timeout is too large")};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(fileDescriptor, point);
    this->wheel[point].emplace(fileDescriptor);
}

auto Timer::update(unsigned int fileDescriptor, unsigned char timeout, source_location sourceLocation) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);

    if (timeout >= this->wheel.size())
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "timeout is too large")};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->wheel[point].emplace(fileDescriptor);
    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(unsigned int fileDescriptor) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, static_cast<int>(this->fileDescriptorIndex), IORING_ASYNC_CANCEL_ALL};

    const UserData userData{TaskType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::setCancelTask(Task &&task) noexcept -> void { this->cancelTask = std::move(task); }

auto Timer::resumeCancel(pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelTask.resume();
}

auto Timer::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{TaskType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}

auto Timer::setCloseTask(Task &&task) noexcept -> void { this->closeTask = std::move(task); }

auto Timer::resumeClose(pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeTask.resume();
}
