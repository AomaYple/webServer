#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

#include <queue>

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, std::shared_ptr<Ring> ring, unsigned long seconds);

    Client(const Client &) = delete;

    auto operator=(const Client &) -> Client & = delete;

    Client(Client &&) noexcept = default;

    auto operator=(Client &&) noexcept -> Client & = default;

    ~Client() = default;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    auto receive() const -> void;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte>;

    auto send(std::vector<std::byte> &&data) -> void;

    auto clearSentData() -> void;

private:
    RingBuffer ringBuffer;
    unsigned long seconds;
    std::queue<std::vector<std::byte>> buffers;
};
