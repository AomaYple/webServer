#pragma once

#include <string>

enum class Event { RECEIVE, SEND, CLOSE };

class Buffer;
class Ring;

class Client {
public:
    Client(int socket, Ring &ring, Buffer &buffer);

    Client(const Client &client) = delete;

    Client(Client &&client) noexcept;

    auto operator=(Client &&client) noexcept -> Client &;

    [[nodiscard]] auto getEvent() const -> Event;

    auto setEvent(Event newEvent) -> void;

    [[nodiscard]] auto getKeepAlive() const -> bool;

    auto setKeepAlive(bool option) -> void;

    auto updateTime(Ring &ring) -> void;

    auto receive(Ring &ring, Buffer &buffer) -> void;

    auto send(std::string &&data, Ring &ring) -> void;

    auto close(Ring &ring) -> void;

private:
    auto time(Ring &ring) -> void;

    auto cancel(Ring &ring) -> void;

    int self;
    Event event;
    unsigned short timeout;
    bool keepAlive;
    std::string sendData;
};
