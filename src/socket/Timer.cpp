#include "Timer.hpp"

#include "../log/Exception.hpp"
#include "../log/logger.hpp"
#include "../userRing/Submission.hpp"

#include <liburing.h>
#include <sys/timerfd.h>

#include <cstring>

Timer::Timer(int fileDescriptorIndex) noexcept
    : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0}, awaiter{} {}

auto Timer::create() noexcept -> int {
    const int fileDescriptor{Timer::createFileDescriptor()};

    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

auto Timer::getFileDescriptorIndex() const noexcept -> int { return this->fileDescriptorIndex; }

auto Timer::clearTimeout() noexcept -> std::vector<int> {
    std::vector<int> timeoutFileDescriptors;

    while (this->expireCount > 0) {
        std::unordered_set<int> &fileDescriptors{this->wheel[this->now]};

        for (const int fileDescriptor: fileDescriptors) {
            timeoutFileDescriptors.emplace_back(fileDescriptor);

            this->location.erase(fileDescriptor);
        }

        fileDescriptors.clear();
        this->now = (this->now + 1) % this->wheel.size();
        --this->expireCount;
    }

    return timeoutFileDescriptors;
}

auto Timer::add(int fileDescriptor, unsigned long seconds) noexcept -> void {
    const unsigned long point{(this->now + seconds) % this->wheel.size()};

    this->wheel[point].emplace(fileDescriptor);
    this->location.emplace(fileDescriptor, point);
}

auto Timer::update(int fileDescriptor, unsigned long seconds) noexcept -> void {
    const unsigned long point{(this->now + seconds) % this->wheel.size()};

    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->wheel[point].emplace(fileDescriptor);

    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(int fileDescriptor) noexcept -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::setAwaiterOutcome(Outcome outcome) noexcept -> void { this->awaiter.setOutcome(outcome); }

auto Timer::setTimingGenerator(Generator &&generator) noexcept -> void { this->timingGenerator = std::move(generator); }

auto Timer::getTimingSubmission() noexcept -> Submission {
    return Submission{Event{Event::Type::timing, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
                      Submission::ReadParameters{std::as_writable_bytes(std::span{&this->expireCount, 1}), 0}};
}

auto Timer::timing() const noexcept -> const Awaiter & { return this->awaiter; }

auto Timer::resumeTiming() const -> void { this->timingGenerator.resume(); }

auto Timer::setCancelGenerator(Generator &&generator) noexcept -> void { this->cancelGenerator = std::move(generator); }

auto Timer::getCancelSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::cancel, this->fileDescriptorIndex}, IOSQE_FIXED_FILE,
                      Submission::CancelParameters{IORING_ASYNC_CANCEL_ALL}};
}

auto Timer::cancel() const noexcept -> const Awaiter & { return this->awaiter; }

auto Timer::resumeCancel() const -> void { this->cancelGenerator.resume(); }

auto Timer::setCloseGenerator(Generator &&generator) noexcept -> void { this->closeGenerator = std::move(generator); }

auto Timer::getCloseSubmission() const noexcept -> Submission {
    return Submission{Event{Event::Type::close, this->fileDescriptorIndex}, 0, Submission::CloseParameters{}};
}

auto Timer::close() const noexcept -> const Awaiter & { return this->awaiter; }

auto Timer::resumeClose() const -> void { this->closeGenerator.resume(); }

auto Timer::createFileDescriptor(std::source_location sourceLocation) noexcept -> int {
    const int fileDescriptor{timerfd_create(CLOCK_MONOTONIC, 0)};
    if (fileDescriptor == -1) {
        logger::push(Log{Log::Level::error, std::strerror(errno), sourceLocation});

        std::terminate();
    }

    return fileDescriptor;
}

auto Timer::setTime(int fileDescriptor, std::source_location sourceLocation) noexcept -> void {
    static constexpr itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(fileDescriptor, 0, &time, nullptr) == -1) {
        logger::push(Log{Log::Level::error, std::strerror(errno), sourceLocation});

        std::terminate();
    }
}
