#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

#include <chrono>

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, std::shared_ptr<Ring> ring, RingBuffer &&ringBuffer,
           std::chrono::seconds seconds) noexcept;

    Client(const Client &) = delete;

    auto operator=(const Client &) -> Client & = delete;

    Client(Client &&) noexcept = default;

    auto operator=(Client &&) -> Client & = delete;

    ~Client() = default;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte>;

    [[nodiscard]] auto getSeconds() const noexcept -> std::chrono::seconds;

    [[nodiscard]] auto getBuffer() noexcept -> std::vector<std::byte> &;

    auto receive() const -> void;

    auto send() const -> void;

private:
    RingBuffer ringBuffer;
    const std::chrono::seconds seconds;
    std::vector<std::byte> buffer;
};
