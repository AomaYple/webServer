#include "Timer.h"

#include <sys/timerfd.h>

#include <cstring>

#include "../base/Submission.h"
#include "../log/Log.h"
#include "../network/UserData.h"

using std::runtime_error, std::out_of_range;
using std::source_location;
using std::string;

Timer::Timer() : fileDescriptor{}, now{0}, expireCount{0} {
    this->create();

    this->setTime();
}

Timer::Timer(Timer &&other)

        noexcept
    : fileDescriptor{other.fileDescriptor}, now{other.now}, expireCount{other.expireCount},
      wheel{std::move(other.wheel)}, location{std::move(other.location)} {
    other.fileDescriptor = -1;
}

auto Timer::operator=(Timer &&other)

        noexcept -> Timer & {
    if (this != &other) {
        this->fileDescriptor = other.fileDescriptor;
        this->now = other.now;
        this->expireCount = other.expireCount;
        this->wheel = std::move(other.wheel);
        this->location = std::move(other.location);
        other.fileDescriptor = -1;
    }
    return *this;
}

auto Timer::start(io_uring_sqe *sqe)

        noexcept -> void {
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

auto Timer::add(Client &&client) -> void {
    unsigned short timeout{client.getTimeout()};
    if (timeout >= this->wheel.size()) throw out_of_range("timeout is too large");

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
    } catch (const runtime_error &error) { Log::produce(source_location::current(), Level::ERROR, error.what()); }
}

auto Timer::create() -> void {
    this->fileDescriptor = timerfd_create(CLOCK_BOOTTIME, 0);

    if (this->fileDescriptor == -1)
        throw runtime_error("timer create file descriptor error: " + string{std::strerror(errno)});
}

auto Timer::setTime() const -> void {
    itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(this->fileDescriptor, 0, &time, nullptr) == -1)
        throw runtime_error("timer initialize error: " + string{std::strerror(errno)});
}

auto Timer::close() const -> void {
    if (::close(this->fileDescriptor) == -1) throw runtime_error("timer close error: " + string{std::strerror(errno)});
}
