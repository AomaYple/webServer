#pragma once

#include "FileDescriptor.hpp"

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, unsigned long seconds) noexcept;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    [[nodiscard]] auto receive(int ringBufferId) const noexcept -> Awaiter;

    [[nodiscard]] auto send(std::span<const std::byte> data) const noexcept -> Awaiter;

private:
    unsigned long seconds;
};
