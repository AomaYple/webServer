#pragma once

#include "../base/BufferRing.h"
#include "../coroutine/Generator.h"
#include "../database/Database.h"
#include "../network/Client.h"
#include "../network/Server.h"
#include "../network/Timer.h"

class EventLoop {
public:
    EventLoop() noexcept;

    EventLoop(const EventLoop &) = delete;

    EventLoop(EventLoop &&) noexcept;

    [[noreturn]] auto loop() noexcept -> void;

private:
    [[nodiscard]] auto accept() noexcept -> Generator;

    [[nodiscard]] auto timing() noexcept -> Generator;

    [[nodiscard]] auto receive(unsigned int fileDescriptor,
                               std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto send(unsigned int fileDescriptor) noexcept -> Generator;

    [[nodiscard]] auto cancel(unsigned int fileDescriptor) noexcept -> Generator;

    [[nodiscard]] auto close(unsigned int fileDescriptor) noexcept -> Generator;

public:
    ~EventLoop();

private:
    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static std::vector<int> cpus;
    static constexpr unsigned short ringEntries{1024}, bufferRingBufferSize{1024}, bufferRingEntries{1024}, port{9999};
    static constexpr unsigned char bufferRingId{0};

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
    std::unordered_map<unsigned int, Client> clients;
    std::array<std::unordered_map<unsigned int, Generator>, 6> generators;
};
