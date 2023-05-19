#include "Timer.h"

#include <sys/timerfd.h>

#include <cstring>

#include "Log.h"

using std::string, std::shared_ptr, std::source_location;

Timer::Timer(const source_location &sourceLocation)
    : fileDescriptor{timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)}, now{0} {
    if (this->fileDescriptor == -1)
        Log::add(sourceLocation, Level::ERROR, "Timer create error: " + string{strerror(errno)});

    itimerspec time{{1, 0}, {1, 0}};

    if (timerfd_settime(this->fileDescriptor, 0, &time, nullptr) == -1)
        Log::add(sourceLocation, Level::ERROR, "Timer setTime error: " + string{strerror(errno)});
}

Timer::Timer(Timer &&timer) noexcept
    : fileDescriptor{timer.fileDescriptor},
      now{timer.now},
      wheel{std::move(timer.wheel)},
      location{std::move(timer.location)} {
    timer.fileDescriptor = -1;
    timer.now = 0;
}

auto Timer::operator=(Timer &&timer) noexcept -> Timer & {
    if (this != &timer) {
        this->fileDescriptor = timer.fileDescriptor;
        this->now = timer.now;
        this->wheel = std::move(timer.wheel);
        this->location = std::move(timer.location);
        timer.fileDescriptor = -1;
        timer.now = 0;
    }
    return *this;
}

auto Timer::add(const shared_ptr<Client> &client) -> void {
    unsigned short clientLocation{static_cast<unsigned short>(this->now + client->getTimeout())};

    if (clientLocation >= this->wheel.size()) clientLocation -= this->wheel.size();

    this->location.emplace(client->get(), clientLocation);

    this->wheel[clientLocation].emplace(client->get(), client);
}

auto Timer::find(int clientFileDescriptor) -> shared_ptr<Client> {
    auto clientLocation{this->location.find(clientFileDescriptor)};

    if (clientLocation != this->location.end())
        return this->wheel[clientLocation->second][clientLocation->first];
    else
        return nullptr;
}

auto Timer::reset(const shared_ptr<Client> &client) -> void {
    unsigned short clientLocation{static_cast<unsigned short>(this->now + client->getTimeout())};

    if (clientLocation >= this->wheel.size()) clientLocation -= this->wheel.size();

    if (this->location[client->get()] != clientLocation) {
        this->wheel[this->location[client->get()]].erase(client->get());

        this->wheel[clientLocation].emplace(client->get(), client);

        this->location[client->get()] = clientLocation;
    }
}

auto Timer::remove(const shared_ptr<Client> &client) -> void {
    this->wheel[this->location[client->get()]].erase(client->get());

    this->location.erase(client->get());
}

auto Timer::handleReadableEvent(const source_location &sourceLocation) -> void {
    uint64_t expireNumber{0};

    if (read(this->fileDescriptor, &expireNumber, sizeof(expireNumber)) == sizeof(expireNumber)) {
        while (expireNumber > 0) {
            auto &expireClients{this->wheel[this->now++]};

            if (this->now == this->wheel.size()) this->now -= this->wheel.size();

            expireClients.clear();

            --expireNumber;
        }
    } else
        Log::add(sourceLocation, Level::ERROR, "Timer read error: " + string{strerror(errno)});
}

auto Timer::get() const -> int { return this->fileDescriptor; }

Timer::~Timer() {
    for (auto moment{this->wheel.begin()}; moment != this->wheel.end(); ++moment) moment->clear();

    if (this->fileDescriptor != -1 && close(this->fileDescriptor) == -1)
        Log::add(source_location::current(), Level::ERROR, "Timer close error: " + string{strerror(errno)});
}
