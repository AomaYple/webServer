#pragma once

#include "../base/UserRing.h"

#include <memory>

class Client {
public:
    Client(unsigned int fileDescriptorIndex, unsigned char timeout, const std::shared_ptr<UserRing> &userRing) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned char;

    auto receive(unsigned short bufferRingId) const -> void;

    auto writeReceivedData(std::span<const std::byte> data) -> void;

    auto readReceivedData() noexcept -> std::vector<std::byte>;

    auto send(std::vector<std::byte> &&data) -> void;

    ~Client();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    const unsigned int fileDescriptorIndex;
    const unsigned char timeout;
    std::vector<std::byte> receivedData, unSendData;
    std::shared_ptr<UserRing> userRing;
};
