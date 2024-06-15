#pragma once

#include "FileDescriptor.hpp"

#include <chrono>

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, std::chrono::seconds seconds) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) = default;

    auto operator=(const Client &) -> Client & = delete;

    auto operator=(Client &&) -> Client & = delete;

    ~Client() = default;

    [[nodiscard]] auto getSeconds() const noexcept -> std::chrono::seconds;

    [[nodiscard]] auto receive(int ringBufferId) const noexcept -> Awaiter;

    [[nodiscard]] auto send(std::span<const std::byte> data) const noexcept -> Awaiter;

private:
    std::chrono::seconds seconds;
};
