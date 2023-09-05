#include "Timer.h"

#include "../base/Submission.h"
#include "../base/UserData.h"

#include <sys/timerfd.h>

#include <algorithm>

using namespace std;

auto Timer::create() noexcept -> unsigned int {
    const unsigned int fileDescriptor{Timer::createFileDescriptor()};

    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

Timer::Timer(unsigned int fileDescriptorIndex) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0} {}

Timer::Timer(Timer &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, now{other.now}, expireCount{other.expireCount},
      wheel{std::move(other.wheel)}, location{std::move(other.location)}, awaiter{std::move(other.awaiter)} {}

auto Timer::createFileDescriptor() noexcept -> unsigned int {
    const int fileDescriptor{timerfd_create(CLOCK_BOOTTIME, 0)};

    if (fileDescriptor == -1) terminate();

    return fileDescriptor;
}

auto Timer::setTime(unsigned int fileDescriptor) noexcept -> void {
    const itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(static_cast<int>(fileDescriptor), 0, &time, nullptr) == -1) terminate();
}

auto Timer::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Timer::setResult(pair<int, unsigned int> result) noexcept -> void { this->awaiter.setResult(result); }

auto Timer::timing(io_uring_sqe *sqe) noexcept -> const Awaiter & {
    const unsigned long offset{0};
    const Submission submission{sqe,
                                this->fileDescriptorIndex,
                                {reinterpret_cast<byte *>(&this->expireCount), sizeof(this->expireCount)},
                                offset};

    const UserData userData{EventType::Timeout, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::clearTimeout() noexcept -> vector<unsigned int> {
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

auto Timer::add(unsigned int fileDescriptor, unsigned char timeout) noexcept -> void {
    if (timeout >= this->wheel.size()) terminate();

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(fileDescriptor, point);

    this->wheel[point].emplace(fileDescriptor);
}

auto Timer::update(unsigned int fileDescriptor, unsigned char timeout) noexcept -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);

    if (timeout >= this->wheel.size()) terminate();

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->wheel[point].emplace(fileDescriptor);
    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(unsigned int fileDescriptor) noexcept -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{EventType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{EventType::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    return this->awaiter;
}
