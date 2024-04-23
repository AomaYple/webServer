#pragma once

#include "../fileDescriptor/Logger.hpp"
#include "../fileDescriptor/Server.hpp"
#include "../fileDescriptor/Timer.hpp"
#include "../http/HttpParse.hpp"
#include "../ring/RingBuffer.hpp"

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

    auto frame() -> void;

    auto submit(std::shared_ptr<Task> &&task) -> void;

    auto eraseCurrentTask() -> void;

    [[nodiscard]] auto write(std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto accept(std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto timing(std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto receive(const Client &client,
                               std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto send(const Client &client, std::vector<std::byte> &&data,
                            std::source_location sourceLocation = std::source_location::current()) -> Task;

    [[nodiscard]] auto cancel(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
        -> Task;

    [[nodiscard]] auto close(int fileDescriptor, std::source_location sourceLocation = std::source_location::current())
        -> Task;

    auto closeAll() -> void;

    static constinit thread_local bool instance;
    static constinit std::mutex lock;
    static constinit int sharedRingFileDescriptor;
    static std::vector<int> ringFileDescriptors;
    static constinit std::atomic_flag switcher;

    const std::shared_ptr<Ring> ring{Scheduler::initializeRing()};
    const std::shared_ptr<Logger> logger{std::make_shared<Logger>(0)};
    const Server server{1};
    Timer timer{2};
    HttpParse httpParse{this->logger};
    std::unordered_map<int, Client> clients;
    RingBuffer ringBuffer{static_cast<unsigned int>(std::bit_ceil(2048 / Scheduler::ringFileDescriptors.size())), 1024,
                          0, this->ring};
    std::unordered_map<unsigned long, std::shared_ptr<Task>> tasks;
    unsigned long currentUserData{};
};
