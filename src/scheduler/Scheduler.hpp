#pragma once

#include "../database/Database.hpp"
#include "../socket/Client.hpp"
#include "../socket/Server.hpp"
#include "../socket/Timer.hpp"
#include "../userRing/BufferRing.hpp"

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
    static constinit int sharedFileDescriptor;
    static constinit std::atomic_ushort cpuCode;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
    std::unordered_map<unsigned int, Client> clients;
};