#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, RingBuffer &&ringBuffer, unsigned long seconds) noexcept;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    [[nodiscard]] auto receive() const noexcept -> Awaiter;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int size) -> std::vector<std::byte>;

    [[nodiscard]] auto send(std::span<const std::byte> data) const noexcept -> Awaiter;

private:
    RingBuffer ringBuffer;
    unsigned long seconds;
};
