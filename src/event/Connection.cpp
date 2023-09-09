#include "Connection.h"

Connection::Connection(Client &&client) noexcept : client{std::move(client)}, generator{nullptr} {}

Connection::Connection(Connection &&other) noexcept
    : client{std::move(other.client)}, generator{std::move(other.generator)}, buffer{std::move(other.buffer)} {}
