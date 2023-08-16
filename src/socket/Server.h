#pragma once

#include "../base/UserRing.h"

#include <memory>

class Server {
public:
    Server(std::uint_least16_t port, const std::shared_ptr<UserRing> &userRing);

    Server(const Server &) = delete;

    Server(Server &&) noexcept;

private:
    [[nodiscard]] static auto socket(std::source_location sourceLocation = std::source_location::current()) -> int;

    auto setSocketOption(std::source_location sourceLocation = std::source_location::current()) const -> void;

    auto bind(std::uint_least16_t port, std::source_location sourceLocation = std::source_location::current()) const
            -> void;

    auto listen(std::source_location sourceLocation = std::source_location::current()) const -> void;

public:
    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    auto setFileDescriptor(int newFileDescriptor) noexcept -> void;

    auto accept() const -> void;

    ~Server();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    int fileDescriptor;
    std::shared_ptr<UserRing> userRing;
};
