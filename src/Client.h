#pragma once

#include "Buffer.h"

#include <string>
#include <source_location>

class Client {
public:
    Client(int fileDescriptor, std::string information);

    Client(const Client &client) = delete;

    Client(Client &&client) noexcept;

    auto operator=(Client &&client) noexcept -> Client &;

    [[nodiscard]] auto send(std::source_location sourceLocation = std::source_location::current()) -> uint32_t;

    [[nodiscard]] auto receive(std::source_location sourceLocation = std::source_location::current()) -> uint32_t;

    auto write(const std::string_view &data) -> void;

    [[nodiscard]] auto read() -> std::string;

    [[nodiscard]] auto get() const -> int;

    [[nodiscard]] auto getExpire() const -> unsigned int;

    auto setExpire(unsigned int time) -> void;

    ~Client();
private:
    int self;
    unsigned int timeout;
    std::string information;
    Buffer sendBuffer, receiveBuffer;
};
