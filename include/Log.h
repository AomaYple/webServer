#pragma once

#include <source_location>
#include <thread>

enum class Level { INFO, WARN, ERROR };

class Log {
public:
    static auto add(std::source_location sourceLocation, Level level, std::string &&message) -> void;

    static auto stopWork() -> void;

    Log(const Log &other) = delete;

    Log(Log &&other) = delete;

private:
    Log();

    struct Node {
        struct Data {
            Data();

            Data(std::chrono::system_clock::time_point time, std::jthread::id threadId,
                 std::source_location sourceLocation, Level level, std::string &&message);

            Data(const Data &other) = default;

            Data(Data &&other) noexcept;

            auto operator=(Data &&other) noexcept -> Data &;

            std::chrono::system_clock::time_point time;
            std::jthread::id threadId;
            std::source_location sourceLocation;
            Level level;
            std::string message;
        };

        Node();

        Node(Data &&data, Node *next);

        Node(const Node &other) = delete;

        Node(Node &&other) noexcept;

        auto operator=(Node &&other) noexcept -> Node &;

        Data data;
        Node *next;
    };

    static Log instance;

    Node *head;
    std::atomic<Node *> tail;
    bool stop;
    std::atomic_flag notice;
    std::jthread work;
};
