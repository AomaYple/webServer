#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, std::shared_ptr<Ring> ring, RingBuffer &&ringBuffer, unsigned long seconds) noexcept;

    Client(const Client &) = delete;

    auto operator=(const Client &) -> Client & = delete;

    Client(Client &&) noexcept = default;

    auto operator=(Client &&) noexcept -> Client & = default;

    ~Client() = default;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte>;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    [[nodiscard]] auto getBuffer() noexcept -> std::vector<std::byte> &;

    auto receive() const -> void;

    auto send() const -> void;

private:
    RingBuffer ringBuffer;
    unsigned long seconds;
    std::vector<std::byte> buffer;
};
