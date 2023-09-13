#pragma once

#include "../database/Database.h"
#include "../socket/Client.h"
#include "../socket/Server.h"
#include "../socket/Timer.h"
#include "../userRing/BufferRing.h"

class Scheduler {
public:
    Scheduler();

    Scheduler(const Scheduler &) = delete;

    Scheduler(Scheduler &&) noexcept;

    [[noreturn]] auto run() -> void;

private:
    auto frame(io_uring_cqe *cqe) -> void;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto receive(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Task;

    [[nodiscard]] auto send(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Task;

    [[nodiscard]] auto cancel(Client &client,
                              std::source_location sourceLocation = std::source_location::current()) const -> Task;

    [[nodiscard]] auto close(const Client &client,
                             std::source_location sourceLocation = std::source_location::current()) const -> Task;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int32_t sharedFileDescriptor;
    static constinit std::atomic_uint16_t cpuCode;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
    std::unordered_map<uint32_t, Client> clients;
};
