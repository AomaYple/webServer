#pragma once

#include <sys/epoll.h>

#include <source_location>
#include <vector>

class Epoll {
public:
    explicit Epoll(const std::source_location &sourceLocation = std::source_location::current());

    Epoll(const Epoll &epoll) = delete;

    Epoll(Epoll &&epoll) noexcept;

    auto operator=(Epoll &&epoll) noexcept -> Epoll &;

    [[nodiscard]] auto poll(bool block = true,
                            const std::source_location &sourceLocation = std::source_location::current())
        -> std::pair<const std::vector<epoll_event> &, unsigned short>;

    auto add(int newFileDescriptor, uint32_t event,
             const std::source_location &sourceLocation = std::source_location::current()) const -> void;

    ~Epoll();

private:
    int fileDescriptor;
    std::vector<epoll_event> epollEvents;
};
