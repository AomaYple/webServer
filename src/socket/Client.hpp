#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Generator.hpp"
#include "../userRing/BufferRing.hpp"

class Submission;

class Client {
public:
    Client(int fileDescriptorIndex, BufferRing &&bufferRing, unsigned long seconds) noexcept;

    Client(const Client &) = delete;

    auto operator=(const Client &) -> Client & = delete;

    Client(Client &&) = default;

    auto operator=(Client &&) -> Client & = delete;

    ~Client() = default;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> int;

    [[nodiscard]] auto getBufferRingData(unsigned short bufferIndex, unsigned int dataSize) noexcept
            -> std::vector<std::byte>;

    [[nodiscard]] auto getSeconds() const noexcept -> unsigned long;

    auto writeToBuffer(std::span<const std::byte> data) noexcept -> void;

    [[nodiscard]] auto readFromBuffer() const noexcept -> std::span<const std::byte>;

    auto clearBuffer() noexcept -> void;

    auto setAwaiterOutcome(Outcome outcome) noexcept -> void;

    auto setReceiveGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getReceiveSubmission() const noexcept -> Submission;

    [[nodiscard]] auto receive() const noexcept -> const Awaiter &;

    auto resumeReceive() const -> void;

    auto setSendGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getSendSubmission() const noexcept -> Submission;

    [[nodiscard]] auto send() const noexcept -> const Awaiter &;

    auto resumeSend() const -> void;

    auto setCancelGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCancelSubmission() const noexcept -> Submission;

    [[nodiscard]] auto cancel() const noexcept -> const Awaiter &;

    auto resumeCancel() const -> void;

    auto setCloseGenerator(Generator &&generator) noexcept -> void;

    [[nodiscard]] auto getCloseSubmission() const noexcept -> Submission;

    [[nodiscard]] auto close() const noexcept -> const Awaiter &;

    auto resumeClose() const -> void;

private:
    const int fileDescriptorIndex;
    BufferRing bufferRing;
    const unsigned long seconds;
    std::vector<std::byte> buffer;
    Generator receiveGenerator, sendGenerator, cancelGenerator, closeGenerator;
    Awaiter awaiter;
};
