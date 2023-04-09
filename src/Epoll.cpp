#include "Epoll.h"
#include "Log.h"

#include <cstring>

using std::string, std::vector, std::pair, std::source_location;

Epoll::Epoll(source_location sourceLocation) : self(epoll_create1(0)), epollEvents(1024) {
    if (this->self == -1)
        Log::add(sourceLocation, Level::ERROR, "Epoll create error: " + string(strerror(errno)));
}

Epoll::Epoll(Epoll &&epoll) noexcept : self(epoll.self), epollEvents(std::move(epoll.epollEvents)) {
    epoll.self = -1;
}

auto Epoll::operator=(Epoll &&epoll) noexcept -> Epoll & {
    if (this != &epoll) {
        this->self = epoll.self;
        this->epollEvents = std::move(epoll.epollEvents);
        epoll.self = -1;
    }
    return *this;
}

auto Epoll::get() const -> int {
    return this->self;
}

auto Epoll::add(int fileDescriptor, uint32_t event, source_location sourceLocation) const -> void {
    epoll_event epollEvent {event};
    epollEvent.data.fd = fileDescriptor;

    if (epoll_ctl(this->self, EPOLL_CTL_ADD, fileDescriptor, &epollEvent) == -1)
        Log::add(sourceLocation, Level::ERROR, "Epoll add error: " + string(strerror(errno)));
}

auto Epoll::mod(int fileDescriptor, uint32_t event, source_location sourceLocation) const -> void {
    epoll_event epollEvent {event};
    epollEvent.data.fd = fileDescriptor;

    if (epoll_ctl(this->self, EPOLL_CTL_MOD, fileDescriptor, &epollEvent) == -1)
        Log::add(sourceLocation, Level::ERROR, "Epoll mod error: " + string(strerror(errno)));
}

auto Epoll::poll(bool block, source_location sourceLocation) -> pair<const vector<epoll_event> &, unsigned int> {
    int eventNumber {epoll_wait(this->self, this->epollEvents.data(), static_cast<int>(this->epollEvents.size()),
                                block ? -1 : 0)};

    if (eventNumber == -1)
        Log::add(sourceLocation, Level::ERROR, "Epoll poll error: " + string(strerror(errno)));
    else if (eventNumber == this->epollEvents.size())
        this->epollEvents.resize(this->epollEvents.size() * 2);

    return {this->epollEvents, eventNumber};
}

Epoll::~Epoll() {
    if (this->self != -1 && close(this->self) == -1)
        Log::add(source_location::current(), Level::ERROR, "Epoll close error: " + string(strerror(errno)));
}
