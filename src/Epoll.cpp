#include "Epoll.h"

#include <cstring>

#include "Log.h"

using std::string, std::span, std::source_location, std::runtime_error;

Epoll::Epoll() : fileDescriptor{epoll_create1(0)}, epollEvents{1024} {
    if (this->fileDescriptor == -1) throw runtime_error("epoll create error: " + string{std::strerror(errno)});
}

Epoll::Epoll(Epoll &&other) noexcept : fileDescriptor{other.fileDescriptor}, epollEvents{std::move(other.epollEvents)} {
    other.fileDescriptor = -1;
}

auto Epoll::operator=(Epoll &&epoll) noexcept -> Epoll & {
    if (this != &epoll) {
        this->fileDescriptor = epoll.fileDescriptor;
        this->epollEvents = std::move(epoll.epollEvents);
        epoll.fileDescriptor = -1;
    }
    return *this;
}

auto Epoll::poll() -> span<epoll_event> {
    int eventNumber{
            epoll_wait(this->fileDescriptor, this->epollEvents.data(), static_cast<int>(this->epollEvents.size()), -1)};

    if (eventNumber == -1) throw runtime_error("epoll poll error: " + string{std::strerror(errno)});

    if (eventNumber == this->epollEvents.size()) this->epollEvents.resize(this->epollEvents.size() * 2);

    return {this->epollEvents.begin(), this->epollEvents.begin() + eventNumber};
}

auto Epoll::add(int socket, unsigned int event, source_location sourceLocation) const -> void {
    epoll_event epollEvent{event};

    epollEvent.data.fd = socket;

    if (epoll_ctl(this->fileDescriptor, EPOLL_CTL_ADD, socket, &epollEvent) == -1)
        Log::add(sourceLocation, Level::WARN, "epoll add error: " + string{std::strerror(errno)});
}

Epoll::~Epoll() {
    if (this->fileDescriptor != -1 && close(this->fileDescriptor) == -1)
        Log::add(source_location::current(), Level::ERROR, "epoll close error: " + string{std::strerror(errno)});
}