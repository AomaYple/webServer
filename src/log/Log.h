#pragma once

#include <fstream>
#include <source_location>
#include <thread>

enum class LogLevel : unsigned char { Warn, Error, Fatal };

class Log {
    struct Node {
        Node(std::string_view data, Node *next);

        Node(const Node &) = delete;

        Node(Node &&) noexcept = default;

        std::string data;
        Node *next;
    };

    Log();

public:
    Log(const Log &) = delete;

    Log(Log &&) = delete;

    static auto combine(std::chrono::system_clock::time_point timestamp, std::jthread::id threadId,
                        std::source_location sourceLocation, LogLevel logLevel, std::string_view text) -> std::string;

    static auto produce(std::string_view log) -> void;

private:
    [[noreturn]] auto loop() -> void;

    static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) -> void;

    ~Log();

    static Log instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    const std::jthread work;
};
