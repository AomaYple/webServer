#pragma once

#include "../fileDescriptor/Server.hpp"
#include "../fileDescriptor/Timer.hpp"
#include "../http/HttpParse.hpp"

#include <memory>

class Ring;
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

        auto submit(Task &&task, bool multiShot = {}) -> void;

        [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Task;

        [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Task;

        [[nodiscard]] auto receive(Client &client,
                                   std::source_location sourceLocation = std::source_location::current())
                -> Task;

        [[nodiscard]] auto send(Client &client, std::source_location sourceLocation = std::source_location::current())
                -> Task;

        [[nodiscard]] auto close(int fileDescriptor,
                                 std::source_location sourceLocation = std::source_location::current())
                -> Task;

        static constinit thread_local bool instance;
        static constinit std::mutex lock;
        static constinit int sharedRingFileDescriptor;
        static std::vector<int> ringFileDescriptors;
        static constinit std::atomic_flag switcher;

        const std::shared_ptr<Ring> ring{Scheduler::initializeRing()};
        Server server{0};
        Timer timer{1};
        HttpParse httpParse;
        std::unordered_map<int, Client> clients;
        std::unordered_map<unsigned long, Task> tasks;
};
