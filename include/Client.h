#pragma once

#include <source_location>
#include <string>

#include "Buffer.h"

class Client {
public:
    Client(int socket, std::string &&information, unsigned short timeout = 60);

    Client(const Client &other) = delete;

    Client(Client &&other) noexcept;

    auto operator=(Client &&other) noexcept -> Client &;

    auto receive(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto send(std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto read() -> std::string;

    auto write(std::string_view data) -> void;

    [[nodiscard]] auto get() const -> int;

    [[nodiscard]] auto getEvent() const -> unsigned int;

    [[nodiscard]] auto getTimeout() const -> unsigned short;

    [[nodiscard]] auto getInformation() const -> std::string_view;

    auto setKeepAlive(bool value) -> void;

    ~Client();

private:
    int socket;
    unsigned int event;
    unsigned short timeout;
    bool keepAlive;
    std::string information;
    Buffer receiveBuffer, sendBuffer;
};