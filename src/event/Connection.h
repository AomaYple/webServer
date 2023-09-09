#pragma once

#include "../coroutine/Generator.h"
#include "../network/Client.h"

#include <vector>

struct Connection {
    explicit Connection(Client &&client) noexcept;

    Connection(const Connection &) = delete;

    Connection(Connection &&) noexcept;

    Client client;
    Generator generator;
    std::vector<std::byte> buffer;
};