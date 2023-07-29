#pragma once

#include <fstream>

#include "Node.h"

class Log {
    static Log instance;

public:
    static auto produce(std::source_location sourceLocation, Level level, std::string &&information) -> void;

    Log(const Log &) = delete;

private:
    static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) -> void;

    [[noreturn]] auto loop() -> void;

    Log();

    ~Log();

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    std::jthread work;
};
