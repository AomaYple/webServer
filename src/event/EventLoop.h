#pragma once

#include "../base/BufferRing.h"
#include "../coroutine/Generator.h"
#include "../database/Database.h"
#include "../network/Server.h"
#include "../network/Timer.h"

struct Connection;

class EventLoop {
public:
    EventLoop();

    EventLoop(const EventLoop &) = delete;

    EventLoop(EventLoop &&) noexcept;

    [[noreturn]] auto loop() -> void;

private:
    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto receive(Connection &connection,
                               std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto send(Connection &connection,
                            std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto cancel(Connection &connection,
                              std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto close(Connection &connection,
                             std::source_location sourceLocation = std::source_location::current()) -> Generator;

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
    Generator serverGenerator, timerGenerator;
    std::unordered_map<unsigned int, Connection> connections;
};
