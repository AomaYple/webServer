#pragma once

#include <fstream>
#include <source_location>
#include <thread>

class Log {
public:
    enum class Level : unsigned char { Info, Warn, Error, Fatal };

private:
    struct Node {
        Node(std::string &&log, Node *next) noexcept;

        Node(const Node &) = delete;

        Node(Node &&) = default;

        auto operator=(const Node &) -> Node & = delete;

        auto operator=(Node &&) -> Node & = default;

        ~Node() = default;

        std::string log;
        Node *next;
    };

    Log();

public:
    Log(const Log &) = delete;

    Log(Log &&) = delete;

    auto operator=(const Log &) -> Log & = delete;

    auto operator=(Log &&) -> Log & = delete;

    ~Log();

private:
    [[noreturn]] auto run() -> void;

    [[nodiscard]] static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) -> void;

public:
    [[nodiscard]] static auto formatLog(Level level, std::chrono::system_clock::time_point timestamp,
                                        std::jthread::id jThreadId, std::source_location sourceLocation,
                                        std::string &&text) -> std::string;

    static auto produce(std::string &&log) -> void;

private:
    static Log instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    const std::jthread work;
};
