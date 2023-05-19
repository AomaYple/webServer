#pragma once

#include <source_location>
#include <thread>

enum class Level { INFO, ERROR };

class Log {
public:
    static auto add(std::source_location sourceLocation, Level level, std::string &&information) -> void;

    static auto stopWork() -> void;

    Log(const Log &log) = delete;

    Log(Log &&log) = delete;

private:
    Log();

    struct Node {
        struct Message {
            Message();

            Message(std::chrono::system_clock::time_point time, std::jthread::id threadId,
                    std::source_location sourceLocation, Level level, std::string &&information);

            Message(const Message &message) = default;

            Message(Message &&message) noexcept;

            auto operator=(Message &&message) noexcept -> Message &;
            std::chrono::system_clock::time_point time;
            std::jthread::id threadId;
            std::source_location sourceLocation;
            Level level;
            std::string information;
        };

        Node();

        Node(Message &&data, Node *next);

        Node(const Node &node) = delete;

        Node(Node &&node) noexcept;

        auto operator=(Node &&node) noexcept -> Node &;

        Message data;
        Node *next;
    };

    static Log log;

    Node *head;
    std::atomic<Node *> tail;
    bool stop;
    std::atomic_flag notice;
    std::jthread work;
};
