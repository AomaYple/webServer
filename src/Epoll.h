#pragma once

#include <vector>
#include <source_location>

#include <sys/epoll.h>

class Epoll {
public:
    explicit Epoll(std::source_location sourceLocation = std::source_location::current());

    Epoll(const Epoll &epoll) = delete;

    Epoll(Epoll &&epoll) noexcept;

    auto operator=(Epoll &&epoll) noexcept -> Epoll &;

    auto add(int fileDescriptor, uint32_t event, std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto mod(int fileDescriptor, uint32_t event, std::source_location sourceLocation = std::source_location::current()) const -> void;

    [[nodiscard]] auto poll(bool block = true, std::source_location sourceLocation = std::source_location::current())
    -> std::pair<const std::vector<epoll_event> &, unsigned int>;

    [[nodiscard]] auto get() const -> int;

    ~Epoll();
private:
    int self;
    std::vector<epoll_event> epollEvents;
};
