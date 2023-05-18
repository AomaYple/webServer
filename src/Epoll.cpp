#include "Epoll.h"

#include <cstring>

#include "Log.h"

using std::string, std::vector, std::pair, std::source_location;

Epoll::Epoll(const source_location& sourceLocation)
    : fileDescriptor{epoll_create1(0)}, epollEvents{1024} {
  if (this->fileDescriptor == -1)
    Log::add(sourceLocation, Level::ERROR,
             "Epoll create error: " + string(strerror(errno)));
}

Epoll::Epoll(Epoll&& epoll) noexcept
    : fileDescriptor{epoll.fileDescriptor},
      epollEvents{std::move(epoll.epollEvents)} {
  epoll.fileDescriptor = -1;
}

auto Epoll::operator=(Epoll&& epoll) noexcept -> Epoll& {
  if (this != &epoll) {
    this->fileDescriptor = epoll.fileDescriptor;
    this->epollEvents = std::move(epoll.epollEvents);
    epoll.fileDescriptor = -1;
  }
  return *this;
}

auto Epoll::poll(bool block, const source_location& sourceLocation)
    -> pair<const vector<epoll_event>&, unsigned short> {
  int eventNumber{epoll_wait(this->fileDescriptor, this->epollEvents.data(),
                             static_cast<int>(this->epollEvents.size()),
                             block ? -1 : 0)};

  if (eventNumber == -1) {
    eventNumber = 0;

    Log::add(sourceLocation, Level::ERROR,
             "Epoll poll error: " + string{strerror(errno)});
  } else if (eventNumber == this->epollEvents.size())
    this->epollEvents.resize(this->epollEvents.size() * 2);

  return {this->epollEvents, eventNumber};
}

auto Epoll::add(int newFileDescriptor, uint32_t event,
                const source_location& sourceLocation) const -> void {
  epoll_event epollEvent{event};

  epollEvent.data.fd = newFileDescriptor;

  if (epoll_ctl(this->fileDescriptor, EPOLL_CTL_ADD, newFileDescriptor,
                &epollEvent) == -1)
    Log::add(sourceLocation, Level::ERROR,
             "Epoll add error: " + string{strerror(errno)});
}

auto Epoll::get() const -> int {
  return this->fileDescriptor;
}

Epoll::~Epoll() {
  if (this->fileDescriptor != -1 && close(this->fileDescriptor) == -1)
    Log::add(source_location::current(), Level::ERROR,
             "Epoll close error: " + string{strerror(errno)});
}
