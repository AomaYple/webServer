#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, RingBuffer &&ringBuffer, unsigned long seconds) noexcept;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    [[nodiscard]] auto receive() const noexcept -> Awaiter;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int size) -> std::vector<std::byte>;

    auto writeToBuffer(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto readFromBuffer() const noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    [[nodiscard]] auto send() const noexcept -> Awaiter;

private:
    RingBuffer ringBuffer;
    unsigned long seconds;
    std::vector<std::byte> buffer;
};
