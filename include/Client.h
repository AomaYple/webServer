#pragma once

#include <memory>

#include "Buffer.h"
#include "Ring.h"

class Submission;

class Client {
public:
    Client(int socket, unsigned short timeout, const std::shared_ptr<Ring> &ring);

    Client(const Client &other) = delete;

    Client(Client &&other) noexcept;

    auto operator=(Client &&other) noexcept -> Client &;

    auto receive() -> void;

    [[nodiscard]] auto get() const -> int;

    [[nodiscard]] auto getTimeout() const -> unsigned short;

    ~Client();

private:
    int socket;
    unsigned short timeout;
    Buffer receiveBuffer, sendBuffer;
    std::shared_ptr<Ring> ring;
};
