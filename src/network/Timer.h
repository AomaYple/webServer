#pragma once

#include "Client.h"

class Timer {
public:
    static auto create() -> unsigned int;

    Timer(unsigned int fileDescriptorIndex, const std::shared_ptr<UserRing> &userRing);

    Timer(const Timer &) = delete;

    Timer(Timer &&) noexcept;

private:
    [[nodiscard]] static auto
    createFileDescriptor(std::source_location sourceLocation = std::source_location::current()) -> unsigned int;

    static auto setTime(unsigned int fileDescriptor,
                        std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    auto startTiming() -> void;

    auto clearTimeout() -> void;

    auto add(Client &&client, std::source_location sourceLocation = std::source_location::current()) -> void;
    
    [[nodiscard]] auto get(unsigned int clientFileDescriptorIndex) -> Client &;

    auto update(unsigned int clientFileDescriptorIndex,
                std::source_location sourceLocation = std::source_location::current()) -> void;

    auto remove(unsigned int clientFileDescriptorIndex) -> void;

    ~Timer();

private:
    auto cancel() const -> void;

    auto close() const -> void;

    const unsigned int fileDescriptorIndex;
    unsigned char now;
    unsigned long expireCount;
    std::array<std::unordered_map<unsigned int, Client>, 61> wheel;
    std::unordered_map<unsigned int, unsigned char> location;
    std::shared_ptr<UserRing> userRing;
};