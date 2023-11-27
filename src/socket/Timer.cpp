#include "Timer.hpp"

#include "../log/Exception.hpp"
#include "../userRing/Event.hpp"
#include "../userRing/Submission.hpp"

#include <sys/timerfd.h>

#include <cstring>

Timer::Timer(unsigned int fileDescriptorIndex) : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0} {}

auto Timer::create() -> unsigned int {
    const unsigned int fileDescriptor{Timer::createFileDescriptor()};

    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

auto Timer::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Timer::timing(io_uring_sqe *sqe) noexcept -> const Awaiter & {
    constexpr unsigned long offset{0};
    const Submission submission{sqe,
                                this->fileDescriptorIndex,
                                {std::as_writable_bytes(std::span{&this->expireCount, 1})},
                                offset};

    const Event event{Event::Type::timeout, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::clearTimeout() -> std::vector<unsigned int> {
    std::vector<unsigned int> timeoutFileDescriptors;

    while (this->expireCount > 0) {
        auto &fileDescriptors{this->wheel[this->now]};

        for (const unsigned int fileDescriptor: fileDescriptors) {
            timeoutFileDescriptors.emplace_back(fileDescriptor);

            this->location.erase(fileDescriptor);
        }

        fileDescriptors.clear();

        this->now = (this->now + 1) % this->wheel.size();

        --this->expireCount;
    }

    return timeoutFileDescriptors;
}

auto Timer::add(unsigned int fileDescriptor, unsigned short timeout, std::source_location sourceLocation) -> void {
    if (timeout >= this->wheel.size()) throw Exception{Log{Log::Level::fatal, "timeout is too large", sourceLocation}};

    const unsigned short point{static_cast<unsigned short>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(fileDescriptor, point);
    this->wheel[point].emplace(fileDescriptor);
}

auto Timer::update(unsigned int fileDescriptor, unsigned short timeout, std::source_location sourceLocation) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);

    if (timeout >= this->wheel.size()) throw Exception{Log{Log::Level::fatal, "timeout is too large", sourceLocation}};

    const unsigned short point{static_cast<unsigned short>((this->now + timeout) % this->wheel.size())};

    this->wheel[point].emplace(fileDescriptor);
    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(unsigned int fileDescriptor) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::setTimingGenerator(Generator &&generator) noexcept -> void { this->timingGenerator = std::move(generator); }

auto Timer::resumeTiming(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->timingGenerator.resume();
}

auto Timer::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const Event event{Event::Type::cancel, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::setCancelGenerator(Generator &&generator) noexcept -> void { this->cancelGenerator = std::move(generator); }

auto Timer::resumeCancel(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelGenerator.resume();
}

auto Timer::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const Event event{Event::Type::close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(event));

    return this->awaiter;
}

auto Timer::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Timer::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeGenerator.resume();
}

auto Timer::createFileDescriptor(std::source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{timerfd_create(CLOCK_MONOTONIC, 0)};

    if (fileDescriptor == -1) throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};

    return fileDescriptor;
}

auto Timer::setTime(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    constexpr itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(static_cast<int>(fileDescriptor), 0, &time, nullptr) == -1)
        throw Exception{Log{Log::Level::fatal, std::strerror(errno), sourceLocation}};
}
