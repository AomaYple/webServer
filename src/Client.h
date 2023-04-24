#pragma once

#include "Buffer.h"

#include <string>
#include <source_location>

class Client {
public:
    Client(int fileDescriptor, std::string information, unsigned short timeout = 60,
           const std::source_location &sourceLocation = std::source_location::current());

    Client(const Client &client) = delete;

    Client(Client &&client) noexcept;

    auto operator=(Client &&client) noexcept -> Client &;

    auto send(const std::source_location &sourceLocation = std::source_location::current()) -> void;

    auto receive(const std::source_location &sourceLocation = std::source_location::current()) -> void;

    auto write(const std::string_view &data) -> void;

    [[nodiscard]] auto read() -> std::string;

    [[nodiscard]] auto get() const -> int;

    [[nodiscard]] auto getEvent() const -> uint32_t;

    auto setKeepAlive() -> void;

    [[nodiscard]] auto getTimeout() const -> unsigned short;

    [[nodiscard]] auto getInformation() const -> std::string_view;

    ~Client();
private:
    int self;
    uint32_t event;
    unsigned short timeout;
    bool keepAlive;
    std::string information;
    Buffer sendBuffer, receiveBuffer;
};
