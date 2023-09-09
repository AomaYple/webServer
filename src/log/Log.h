#pragma once

#include <fstream>
#include <source_location>
#include <thread>

class Log {
public:
    enum class Level : unsigned char { Info, Warn, Error, Fatal };

private:
    struct Node {
        Node(std::string_view log, Node *next);

        Node(const Node &) = delete;

        Node(Node &&) noexcept = default;

        const std::string log;
        Node *next;
    };

    Log();

    [[noreturn]] auto run() -> void;

    static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) -> void;

public:
    Log(const Log &) = delete;

    Log(Log &&) = delete;

    static auto formatLog(Level level, std::chrono::system_clock::time_point timestamp, std::jthread::id jThreadId,
                          std::source_location sourceLocation, std::string_view text) -> std::string;

    static auto produce(std::string_view log) -> void;

private:
    ~Log();

    static Log instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    const std::jthread work;
};
