#include "Timer.h"

#include <sys/timerfd.h>

#include <cstring>

#include "Event.h"
#include "Log.h"

using std::runtime_error, std::out_of_range;
using std::source_location;
using std::string;

Timer::Timer() : self{timerfd_create(CLOCK_BOOTTIME, 0)}, now{0}, expireCount{0} {
    if (this->self == -1) throw runtime_error("timer create file descriptor error: " + string{std::strerror(errno)});

    itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(this->self, 0, &time, nullptr) == -1)
        throw runtime_error("timer initialize error: " + string{std::strerror(errno)});
}

Timer::Timer(Timer &&other) noexcept
    : self{other.self}, now{other.now}, expireCount{other.expireCount}, wheel{std::move(other.wheel)},
      location{std::move(other.location)} {
    other.self = -1;
}

auto Timer::operator=(Timer &&other) noexcept -> Timer & {
    if (this != &other) {
        this->self = other.self;
        this->now = other.now;
        this->expireCount = other.expireCount;
        this->wheel = std::move(other.wheel);
        this->location = std::move(other.location);
        other.self = -1;
    }
    return *this;
}

auto Timer::start(Submission &&submission) noexcept -> void {
    Event event{Type::TIMEOUT, this->self};
    submission.setUserData(reinterpret_cast<unsigned long long &>(event));

    submission.read(this->self, &this->expireCount, sizeof(this->expireCount), 0);
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

    int socket{client.get()};
    unsigned short point{static_cast<unsigned short>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(socket, point);

    this->wheel[point].emplace(socket, std::move(client));
}

auto Timer::exist(int socket) const noexcept -> bool { return this->location.contains(socket); }

auto Timer::pop(int socket) -> Client {
    Client client{std::move(this->wheel[this->location.at(socket)].at(socket))};

    this->wheel[this->location.at(socket)].erase(socket);
    this->location.erase(socket);

    return client;
}

Timer::~Timer() {
    try {
        this->close();
    } catch (const runtime_error &error) { Log::produce(source_location::current(), Level::ERROR, error.what()); }
}

auto Timer::close() const -> void {
    if (::close(this->self) == -1) throw runtime_error("timer close error: " + string{std::strerror(errno)});
}
