#include "Timer.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"

#include <sys/timerfd.h>

#include <cstring>

using namespace std;

auto Timer::create() -> unsigned int {
    const unsigned int fileDescriptor{Timer::createFileDescriptor()};

    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

Timer::Timer(unsigned int fileDescriptorIndex, const std::shared_ptr<UserRing> &userRing)
    : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0}, userRing{userRing} {}

Timer::Timer(Timer &&other) noexcept
    : fileDescriptorIndex{other.fileDescriptorIndex}, now{other.now}, expireCount{other.expireCount},
      wheel{std::move(other.wheel)}, location{std::move(other.location)}, userRing{std::move(other.userRing)} {}

auto Timer::createFileDescriptor(source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{timerfd_create(CLOCK_BOOTTIME, 0)};

    if (fileDescriptor == -1)
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, std::strerror(errno))};

    return fileDescriptor;
}

auto Timer::setTime(unsigned int fileDescriptor, source_location sourceLocation) -> void {
    const itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(static_cast<int>(fileDescriptor), 0, &time, nullptr) == -1)
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, std::strerror(errno))};
}

auto Timer::startTiming() -> void {
    unsigned long offset{0};
    const Submission submission{this->userRing->getSqe(),
                                this->fileDescriptorIndex,
                                {reinterpret_cast<byte *>(&this->expireCount), sizeof(this->expireCount)},
                                offset};

    const UserData userData{Type::Timeout, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);
}

auto Timer::clearTimeout() -> void {
    while (this->expireCount > 0) {
        for (auto element{this->wheel[this->now].begin()}; element != this->wheel[this->now].end();) {
            this->location.erase(element->first);

            this->wheel[this->now].erase(element++);
        }

        this->now = (this->now + 1) % this->wheel.size();

        --this->expireCount;
    }
}

auto Timer::add(Client &&client, source_location sourceLocation) -> void {
    const unsigned char timeout{client.getTimeout()};
    if (timeout >= this->wheel.size())
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, "timeout is too large")};

    const unsigned int clientFileDescriptorIndex{client.getFileDescriptorIndex()};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(clientFileDescriptorIndex, point);

    this->wheel[point].emplace(clientFileDescriptorIndex, std::move(client));
}

auto Timer::get(unsigned int clientFileDescriptorIndex) -> Client & {
    return this->wheel[this->location.at(clientFileDescriptorIndex)].at(clientFileDescriptorIndex);
}

auto Timer::update(unsigned int clientFileDescriptorIndex, source_location sourceLocation) -> void {
    Client client{std::move(this->wheel[this->location.at(clientFileDescriptorIndex)].at(clientFileDescriptorIndex))};

    this->wheel[this->location.at(clientFileDescriptorIndex)].erase(clientFileDescriptorIndex);

    const unsigned char timeout{client.getTimeout()};
    if (timeout >= this->wheel.size())
        throw Exception{Log::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                     LogLevel::Fatal, "timeout is too large")};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->wheel[point].emplace(clientFileDescriptorIndex, std::move(client));
    this->location.at(clientFileDescriptorIndex) = point;
}

auto Timer::remove(unsigned int clientFileDescriptorIndex) -> void {
    this->wheel[this->location.at(clientFileDescriptorIndex)].erase(clientFileDescriptorIndex);
    this->location.erase(clientFileDescriptorIndex);
}

Timer::~Timer() {
    if (this->userRing != nullptr) {
        this->cancel();

        this->close();
    }
}

auto Timer::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::Cancel, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Timer::close() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptorIndex};

    const UserData userData{Type::Close, this->fileDescriptorIndex};
    submission.setUserData(reinterpret_cast<const unsigned long &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}