#pragma once

#include <sys/epoll.h>

#include <source_location>
#include <span>
#include <vector>

class Epoll {
public:
    Epoll();

    Epoll(const Epoll &other) = delete;

    Epoll(Epoll &&other) noexcept;

    auto operator=(Epoll &&epoll) noexcept -> Epoll &;

    [[nodiscard]] auto poll() -> std::span<epoll_event>;

    auto add(int socket, unsigned int event,
             std::source_location sourceLocation = std::source_location::current()) const -> void;

    ~Epoll();

private:
    int fileDescriptor;
    std::vector<epoll_event> epollEvents;
};