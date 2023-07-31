#include "Timer.h"

#include <cstring>

#include <sys/timerfd.h>

#include "../base/Submission.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../network/UserData.h"

using std::source_location;

Timer::Timer() : fileDescriptor{Timer::create()}, now{0}, expireCount{0} { this->setTime(); }

auto Timer::start(io_uring_sqe *sqe) noexcept -> void {
    Submission submission{sqe};

    UserData userData{Type::TIMEOUT, this->fileDescriptor};
    submission.setUserData(reinterpret_cast<unsigned long long &>(userData));

    submission.read(this->fileDescriptor, &this->expireCount, sizeof(this->expireCount), 0);
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
    unsigned short timeout{client.getTimeout()};
    if (timeout >= this->wheel.size()) throw Exception{sourceLocation, Level::ERROR, "timeout is too large"};

    int clientFileDescriptor{client.getFileDescriptor()};
    unsigned short point{static_cast<unsigned short>((this->now + timeout) % this->wheel.size())};

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
    try {
        this->close();
    } catch (Exception &exception) { Log::produce(exception.getMessage()); }
}

auto Timer::create(source_location sourceLocation) -> int {
    int fileDescriptor{timerfd_create(CLOCK_BOOTTIME, 0)};

    if (fileDescriptor == -1) throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};

    return fileDescriptor;
}

auto Timer::setTime(source_location sourceLocation) const -> void {
    itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(this->fileDescriptor, 0, &time, nullptr) == -1)
        throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};
}

auto Timer::close(source_location sourceLocation) const -> void {
    if (::close(this->fileDescriptor) == -1) throw Exception{sourceLocation, Level::FATAL, std::strerror(errno)};
}
