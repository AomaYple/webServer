#pragma once

#include "Node.hpp"

#include <fstream>

class Logger {
public:
    Logger(const Logger &) = delete;

    Logger(Logger &&) = delete;

    auto operator=(const Logger &) -> Logger & = delete;

    auto operator=(Logger &&) -> Logger & = delete;

    ~Logger();

    static auto produce(Log &&log) -> void;

    static auto stop() noexcept -> void;

private:
    Logger();

    auto run(std::stop_token stopToken) -> void;

    [[nodiscard]] static auto invertLinkedList(Node *pointer) noexcept -> const Node *;

    auto consume(const Node *pointer) -> void;

    static Logger instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    std::jthread worker;
};
