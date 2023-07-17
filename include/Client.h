#pragma once

#include <memory>

#include "UserRing.h"

class Client {
public:
    Client(int fileDescriptor, unsigned short timeout, const std::shared_ptr<UserRing> &userRing) noexcept;

    Client(const Client &) = delete;

    Client(Client &&) noexcept;

    auto operator=(Client &&) noexcept -> Client &;

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    [[nodiscard]] auto getTimeout() const noexcept -> unsigned short;

    auto receive(unsigned short bufferRingId) -> void;

    auto writeReceivedData(std::string &&data) -> void;

    auto readReceivedData() noexcept -> std::string;

    auto send(std::string &&data) -> void;

    ~Client();

private:
    auto cancel() -> void;

    auto close() -> void;

    int fileDescriptor;
    unsigned short timeout;
    std::string receivedData, unSendData;
    std::shared_ptr<UserRing> userRing;
};
