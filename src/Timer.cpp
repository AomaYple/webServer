#include "Timer.h"

#include <sys/timerfd.h>

#include <cstring>

#include "Log.h"

using std::string, std::shared_ptr, std::source_location, std::runtime_error;

Timer::Timer() : fileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)}, now{0} {
    if (this->fileDescriptor == -1) throw runtime_error("timer create error: " + string{std::strerror(errno)});

    itimerspec time{{1, 0}, {1, 0}};

    if (timerfd_settime(this->fileDescriptor, 0, &time, nullptr) == -1)
        throw runtime_error("timer setTime error: " + string{std::strerror(errno)});
}

Timer::Timer(Timer &&other) noexcept
    : fileDescriptor{other.fileDescriptor}, now{other.now}, location{std::move(other.location)},
      wheel{std::move(other.wheel)} {
    other.fileDescriptor = -1;
}

auto Timer::operator=(Timer &&other) noexcept -> Timer & {
    if (this != &other) {
        this->fileDescriptor = other.fileDescriptor;
        this->now = other.now;
        this->location = std::move(other.location);
        this->wheel = std::move(other.wheel);
        other.fileDescriptor = -1;
    }
    return *this;
}

auto Timer::get() const -> int { return this->fileDescriptor; }

auto Timer::add(const shared_ptr<Client> &client) -> void {
    unsigned short clientLocation{static_cast<unsigned short>(this->now + client->getTimeout())};

    if (clientLocation >= this->wheel.size()) clientLocation -= this->wheel.size();

    this->location.emplace(client->get(), clientLocation);

    this->wheel.at(clientLocation).emplace(client->get(), client);
}

auto Timer::find(int socket) -> shared_ptr<Client> {
    auto clientLocation{this->location.at(socket)};

    return this->wheel.at(clientLocation).at(socket);
}

auto Timer::reset(const shared_ptr<Client> &client) -> void {
    unsigned short clientLocation{static_cast<unsigned short>(this->now + client->getTimeout())};

    int socket{client->get()};

    if (clientLocation >= this->wheel.size()) clientLocation -= this->wheel.size();

    if (this->location.at(socket) != clientLocation) {
        this->wheel.at(this->location.at(socket)).erase(socket);

        this->location.at(socket) = clientLocation;

        this->wheel.at(clientLocation).emplace(socket, client);
    }
}

auto Timer::remove(const shared_ptr<Client> &client) -> void {
    int socket{client->get()};

    this->wheel.at(this->location.at(socket)).erase(socket);

    this->location.erase(socket);
}

auto Timer::handleExpiration() -> void {
    uint64_t expireNumber{0};

    if (read(this->fileDescriptor, &expireNumber, sizeof(expireNumber)) == sizeof(expireNumber)) {
        while (expireNumber > 0) {
            auto &expireClients{this->wheel[this->now++]};

            if (this->now == this->wheel.size()) this->now -= this->wheel.size();

            expireClients.clear();

            --expireNumber;
        }
    } else
        throw runtime_error("timer read error: " + string{std::strerror(errno)});
}

Timer::~Timer() {
    if (this->fileDescriptor != -1) {
        for (auto moment{this->wheel.begin()}; moment != this->wheel.end(); ++moment) moment->clear();

        if (close(this->fileDescriptor) == -1)
            Log::add(source_location::current(), Level::ERROR, "timer close error: " + string{std::strerror(errno)});
    }
}
