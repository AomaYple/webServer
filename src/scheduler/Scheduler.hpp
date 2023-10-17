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

    static auto registerSignal(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto run() -> void;

private:
    static auto initializeUserRing(std::source_location sourceLocation = std::source_location::current())
            -> std::shared_ptr<UserRing>;

    auto cancelAll() -> void;

    auto closeAll() -> void;

    auto frame(const io_uring_cqe *cqe) -> void;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto receive(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    [[nodiscard]] auto send(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    [[nodiscard]] auto cancelClient(Client &client,
                                    std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    [[nodiscard]] auto closeClient(const Client &client,
                                   std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    [[nodiscard]] auto cancelServer(std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    [[nodiscard]] auto closeServer(std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    [[nodiscard]] auto cancelTimer(std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    [[nodiscard]] auto closeTimer(std::source_location sourceLocation = std::source_location::current()) const
            -> Generator;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedUserRingFileDescriptor;
    static std::vector<int> userRingFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<UserRing> userRing;
    BufferRing bufferRing;
    Server server;
    Timer timer;
    Database database;
    std::unordered_map<unsigned int, Client> clients;
};
