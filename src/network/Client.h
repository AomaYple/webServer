#pragma once

#include "../base/UserRing.h"

#include <memory>

class Client {
public:
    Client(std::int_fast32_t fileDescriptor, std::uint_fast16_t timeout,
           const std::shared_ptr<UserRing> &userRing) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    auto operator=(Client &&) noexcept -> Client &;

    [[nodiscard]] auto getFileDescriptor() const noexcept -> std::int_fast32_t;

    [[nodiscard]] auto getTimeout() const noexcept -> std::uint_fast16_t;

    auto receive(std::uint_fast16_t bufferRingId) -> void;

    auto writeReceivedData(std::string &&data) -> void;

    [[nodiscard]] auto readReceivedData() noexcept -> std::string;

    auto send(std::string &&data) -> void;

    ~Client();

private:
    auto cancel() -> void;

    auto close() -> void;

    std::int_fast32_t fileDescriptor;
    std::uint_fast16_t timeout;
    std::string receivedData, unSendData;
    std::shared_ptr<UserRing> userRing;
};
