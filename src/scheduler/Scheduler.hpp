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

    Scheduler(Scheduler &&) = delete;

    auto operator=(const Scheduler &) -> Scheduler & = delete;

    auto operator=(Scheduler &&) -> Scheduler & = delete;

    ~Scheduler();

private:
    static auto judgeOneThreadOneInstance(std::source_location sourceLocation = std::source_location::current())
            -> void;

    auto cancelAll() -> void;

    auto closeAll() -> void;

public:
    [[noreturn]] auto run() -> void;

private:
    auto frame(const io_uring_cqe *cqe) -> void;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto receive(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    [[nodiscard]] auto send(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    [[nodiscard]] auto cancelClient(Client &client,
                                    std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto closeClient(const Client &client,
                                   std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto cancelServer(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto closeServer(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto cancelTimer(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto closeTimer(std::source_location sourceLocation = std::source_location::current()) -> Generator;

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
