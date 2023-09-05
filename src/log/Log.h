#pragma once

#include <fstream>
#include <source_location>
#include <thread>

class Log {
public:
    enum class Level : unsigned char { Info, Warn, Error, Fatal };

private:
    struct Node {
        Node(std::string_view log, Node *next) noexcept;

        Node(const Node &) = delete;

        Node(Node &&) noexcept = default;

        const std::string log;
        Node *next;
    };

    Log() noexcept;

    [[noreturn]] auto run() noexcept -> void;

    [[nodiscard]] static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) noexcept -> void;

public:
    Log(const Log &) = delete;

    Log(Log &&) = delete;

    [[nodiscard]] static auto formatLog(Level level, std::chrono::system_clock::time_point timestamp,
                                        std::jthread::id jThreadId, std::source_location sourceLocation,
                                        std::string_view text) noexcept -> std::string;

    static auto produce(std::string_view log) noexcept -> void;

private:
    ~Log();

    static Log instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    const std::jthread work;
};
