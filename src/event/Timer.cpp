#include "Timer.h"

#include "../base/Submission.h"
#include "../base/UserData.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"

#include <sys/timerfd.h>

#include <cstring>

using std::array, std::span, std::byte, std::shared_ptr, std::source_location;
using std::chrono::system_clock;
using std::this_thread::get_id, std::as_bytes;

Timer::Timer(const shared_ptr<UserRing> &userRing)
    : fileDescriptor{Timer::create()}, now{0}, expireCount{0}, userRing{userRing} {
    this->setTime();
}

Timer::Timer(Timer &&other) noexcept
    : fileDescriptor{other.fileDescriptor}, userRing{std::move(other.userRing)}, now{other.now},
      expireCount{other.expireCount}, wheel{std::move(other.wheel)}, location{std::move(other.location)} {
    other.fileDescriptor = -1;
}

auto Timer::create(source_location sourceLocation) -> int {
    const int fileDescriptor{timerfd_create(CLOCK_BOOTTIME, 0)};

    if (fileDescriptor == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};

    return fileDescriptor;
}

auto Timer::setTime(source_location sourceLocation) const -> void {
    const itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(this->fileDescriptor, 0, &time, nullptr) == -1)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, std::strerror(errno))};
}

auto Timer::getFileDescriptor() const noexcept -> int { return this->fileDescriptor; }

auto Timer::setFileDescriptor(int newFileDescriptor) noexcept -> void { this->fileDescriptor = newFileDescriptor; }

auto Timer::startTiming() -> void {
    const unsigned int offset{0};
    const Submission submission{
            this->userRing->getSqe(),
            this->fileDescriptor,
            {reinterpret_cast<byte *>(&this->expireCount), sizeof(this->expireCount) / sizeof(byte)},
            offset};

    const UserData userData{Type::TIMEOUT, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

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
    const std::uint_least8_t timeout{client.getTimeout()};
    if (timeout >= this->wheel.size())
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, "timeout is too large")};

    const int clientFileDescriptor{client.getFileDescriptor()};

    const std::uint_least8_t point{static_cast<std::uint_least8_t>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(clientFileDescriptor, point);

    this->wheel[point].emplace(clientFileDescriptor, std::move(client));
}

auto Timer::exist(int clientFileDescriptor) const -> bool { return this->location.contains(clientFileDescriptor); }

auto Timer::pop(int clientFileDescriptor) -> Client {
    Client client{std::move(this->wheel[this->location.at(clientFileDescriptor)].at(clientFileDescriptor))};

    this->wheel[this->location.at(clientFileDescriptor)].erase(clientFileDescriptor);
    this->location.erase(clientFileDescriptor);

    return client;
}

Timer::~Timer() {
    if (this->fileDescriptor != -1) {
        try {
            this->cancel();

            this->close();
        } catch (const Exception &exception) { Log::produce(exception.what()); }
    }
}

auto Timer::cancel() const -> void {
    const Submission submission{this->userRing->getSqe(), this->fileDescriptor, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{Type::CANCEL, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS);
}

auto Timer::close() const -> void {
    const Submission submission{this->userRing->getSqe(), static_cast<unsigned int>(this->fileDescriptor)};

    const UserData userData{Type::CLOSE, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<const __u64 &>(userData));

    submission.setFlags(IOSQE_CQE_SKIP_SUCCESS);
}
