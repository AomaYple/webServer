#include "Timer.h"
#include "Log.h"

#include <cstring>

#include <sys/timerfd.h>

using std::string, std::unordered_map, std::shared_ptr, std::source_location;

Timer::Timer(source_location sourceLocation) : self(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)), now(0) {
    if (this->self == -1)
        Log::add(sourceLocation, Level::ERROR, "Timer create error: " + string(strerror(errno)));

    itimerspec time { {1, 0}, {1, 0}};
    if (timerfd_settime(this->self, 0, &time, nullptr) == -1)
        Log::add(sourceLocation, Level::ERROR, "Timer setTime error: " + string(strerror(errno)));
}

Timer::Timer(Timer &&timer) noexcept : self(timer.self), now(timer.now) {
    timer.self = -1;
    timer.now = 0;
}

auto Timer::operator=(Timer &&timer) noexcept -> Timer & {
    if (this != &timer) {
        this->self = timer.self;
        this->now = timer.now;
        timer.self = -1;
        timer.now = 0;
    }
    return *this;
}

auto Timer::add(const shared_ptr<Client>& client) -> void {
    unsigned int location {this->now + client->getExpire()};
    if (location > this->wheel.size() - 1)
        location -= this->wheel.size();

    this->wheel.at(location).emplace_back(client);
    this->table.emplace(client->get(), location);
}

auto Timer::reset(shared_ptr<Client> &client) -> void {
    this->remove(client);
    this->add(client);
}

auto Timer::remove(shared_ptr<Client> &client) -> void {
    this->wheel[this->table.at(client->get())].remove(client);
    this->table.erase(client->get());
}

auto Timer::handleRead(unordered_map<int, shared_ptr<Client>> &clients, source_location sourceLocation) -> void {
    uint64_t number {0};
    if (read(this->self, &number, sizeof(number)) == sizeof(number)) {
        while (number > 0) {
            auto &list {this->wheel[this->now++]};

            while (!list.empty()) {
                int fileDescriptor {list.back()->get()};
                list.pop_back();
                this->table.erase(fileDescriptor);
                clients.erase(fileDescriptor);
            }

            --number;
        }
    } else
        Log::add(sourceLocation, Level::ERROR, "Timer read error: " + string(strerror(errno)));
}

auto Timer::get() const -> int {
    return this->self;
}

Timer::~Timer() {
    if (this->self != -1 && close(this->self) == -1)
        Log::add(source_location::current(), Level::ERROR, "Timer close error: " + string(strerror(errno)));
}
