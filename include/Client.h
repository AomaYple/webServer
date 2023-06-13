#pragma once

#include <memory>

#include "Ring.h"

class BufferRing;

class Client {
public:
    Client(int fileDescriptor, const std::shared_ptr<Ring> &ring, BufferRing &bufferRing, unsigned short timeout = 60);

    Client(const Client &client) = delete;

    Client(Client &&client) noexcept;

    auto operator=(Client &&client) noexcept -> Client &;

    auto receive(BufferRing &bufferRing) -> void;

    auto write(std::string &&data) -> void;

    auto read() -> std::string;

    auto updateTimeout() -> void;

    ~Client();

private:
    auto timeout() -> void;

    auto removeTimeout() -> void;

    auto cancel() -> void;

    auto close() -> void;

    int self;
    unsigned short timeoutTime;
    bool keepAlive;
    std::string receivedData;
    std::shared_ptr<Ring> ring;
};
