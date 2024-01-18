#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"
#include "../ring/RingBuffer.hpp"
#include "FileDescriptor.hpp"

class Client : public FileDescriptor {
public:
    Client(int fileDescriptor, std::shared_ptr<Ring> ring, unsigned long seconds);

    Client(const Client &) = delete;

    auto operator=(const Client &) -> Client & = delete;

    Client(Client &&) = default;

    auto operator=(Client &&) -> Client & = default;

    ~Client() = default;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    auto setFirstGenerator(Generator &&generator) noexcept -> void;

    auto resumeFirstGenerator(Outcome outcome) -> void;

    auto setSecondGenerator(Generator &&generator) noexcept -> void;

    auto resumeSecondGenerator(Outcome outcome) -> void;

    auto startReceive() const -> void;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    [[nodiscard]] auto getReceivedData(unsigned short index, unsigned int dataSize) -> std::vector<std::byte>;

    auto writeToBuffer(std::span<const std::byte> data) -> void;

    [[nodiscard]] auto readFromBuffer() const noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    [[nodiscard]] auto send() const -> const Awaiter &;

private:
    auto setAwaiterOutcome(Outcome outcome) noexcept -> void;

    RingBuffer ringBuffer;
    unsigned long seconds;
    std::vector<std::byte> buffer;
    std::pair<Generator, Generator> generators{nullptr, nullptr};
    Awaiter awaiter{};
};
