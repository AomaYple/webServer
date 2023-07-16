#pragma once

#include "Node.h"

class Log {
    static Log instance;

public:
    static auto produce(std::source_location sourceLocation, Level level, std::string &&information) -> void;

    Log(const Log &other) = delete;

    Log(Log &&other) = delete;

private:
    static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    static auto consume(Node *pointer) -> void;

    Log();

    ~Log();

    std::atomic<Node *> head;
    std::atomic_flag notice;
    std::jthread work;
};
