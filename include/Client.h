#pragma once

#include <memory>

#include "Ring.h"

class Buffer;

class Client {
public:
    Client(int socket, std::shared_ptr<Ring> &ring, Buffer &buffer, unsigned short timeout = 60);

    Client(const Client &client) = delete;

    Client(Client &&client) noexcept;

    auto operator=(Client &&client) noexcept -> Client &;

private:
    auto time() -> void;

public:
    [[nodiscard]] auto getKeepAlive() const -> bool;

    auto setKeepAlive(bool option) -> void;

    auto updateTime() -> void;

    auto receive(Buffer &buffer) -> void;

    auto send(std::string &&data) -> void;

    auto write(std::string &&data) -> void;

    auto read() -> std::string;

private:
    auto cancel() -> void;

    auto close() -> void;

public:
    ~Client();

private:
    int self;
    unsigned short timeout;
    bool keepAlive;
    std::string receivedData, unSendData;
    std::shared_ptr<Ring> ring;
};
