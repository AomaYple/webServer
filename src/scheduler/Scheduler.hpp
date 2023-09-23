#pragma once

#include "../database/Database.hpp"
#include "../socket/Server.hpp"
#include "../socket/Timer.hpp"
#include "../userRing/BufferRing.hpp"

class Client;

class Scheduler {
public:
    Scheduler();

    Scheduler(const Scheduler &) = delete;

    Scheduler(Scheduler &&) = default;

    auto operator=(const Scheduler &) -> Scheduler & = delete;

    auto operator=(Scheduler &&) noexcept -> Scheduler &;

    ~Scheduler();

private:
    auto destroy() -> void;

    static auto judgeOneThreadOneInstance(std::source_location sourceLocation = std::source_location::current())
            -> void;

public:
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
    static constinit int sharedUserRingFileDescriptor;
    static std::vector<int> userRingFileDescriptors;

    std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
    std::unordered_map<unsigned int, Client> clients;
};
