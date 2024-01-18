#pragma once

#include "../fileDescriptor/Server.hpp"
#include "../fileDescriptor/Timer.hpp"
#include "../http/HttpParse.hpp"

class Client;

class Scheduler {
public:
    static auto registerSignal(std::source_location sourceLocation = std::source_location::current()) -> void;

    Scheduler();

    Scheduler(const Scheduler &) = delete;

    auto operator=(const Scheduler &) -> Scheduler & = delete;

    Scheduler(Scheduler &&) = delete;

    auto operator=(Scheduler &&) -> Scheduler & = delete;

    ~Scheduler();

    auto run() -> void;

private:
    [[nodiscard]] static auto initializeRing(std::source_location sourceLocation = std::source_location::current())
            -> std::shared_ptr<Ring>;

    [[nodiscard]] static auto initializeHttpParse() -> HttpParse;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Generator;

    [[nodiscard]] auto receive(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    [[nodiscard]] auto send(Client &client, std::source_location sourceLocation = std::source_location::current())
            -> Generator;

    static auto closed(int fileDescriptor, int result,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedRingFileDescriptor;
    static std::vector<int> ringFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<Ring> ring{Scheduler::initializeRing()};
    Server server{0, this->ring};
    Timer timer{1, this->ring};
    HttpParse httpParse{Scheduler::initializeHttpParse()};
    std::unordered_map<int, Client> clients;
};
