#pragma once

#include "../base/UserRing.h"

#include <memory>
#include <queue>

class Client {
public:
    Client(int fileDescriptor, std::uint_least8_t timeout, const std::shared_ptr<UserRing> &userRing) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    [[nodiscard]] auto getTimeout() const noexcept -> std::uint_least8_t;

    auto receive(__u16 bufferRingId) const -> void;

    auto writeReceivedData(std::vector<std::byte> &&data) -> void;

    [[nodiscard]] auto readReceivedData() noexcept -> std::vector<std::byte>;

    auto writeUnSendData(std::queue<std::vector<std::byte>> &&data) noexcept -> void;

    auto send() -> void;

    ~Client();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    int fileDescriptor;
    const std::uint_least8_t timeout;
    std::vector<std::byte> receivedData;
    std::queue<std::vector<std::byte>> unSendData;
    std::shared_ptr<UserRing> userRing;
};
