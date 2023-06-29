#pragma once

#include <memory>

#include "UserRing.h"

class Client {
public:
    Client(int socket, const std::shared_ptr<UserRing> &userRing) noexcept;

    Client(const Client &other) = delete;

    Client(Client &&other) noexcept;

    auto operator=(Client &&other) noexcept -> Client &;

    auto receive(unsigned short bufferRingId) -> void;

    auto writeReceivedData(std::string &&data) noexcept -> void;

    auto readReceivedData() noexcept -> std::string;

    auto send(std::string &&data) -> void;

    ~Client();

private:
    auto cancel() -> void;

    auto close() -> void;

    int socket;
    std::string receivedData, unSendData;
    std::shared_ptr<UserRing> userRing;
};
