#pragma once

#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

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

    [[nodiscard]] auto getBuffer() noexcept -> std::vector<std::byte> &;

    auto send() -> void;

private:
    RingBuffer ringBuffer;
    unsigned long seconds;
    std::vector<std::byte> buffer;
};
