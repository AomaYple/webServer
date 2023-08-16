#pragma once

#include "Node.h"

#include <fstream>
#include <thread>

class Log {
    Log();

public:
    Log(const Log &) = delete;

    Log(Log &&) = delete;

    static auto produce(std::string &&log) -> void;

private:
    [[noreturn]] auto loop() -> void;

    [[nodiscard]] static auto invertLinkedList(Node *pointer) noexcept -> Node *;

    auto consume(Node *pointer) -> void;

    ~Log();

    static Log instance;

    std::ofstream logFile;
    std::atomic<Node *> head;
    std::atomic_flag notice;
    const std::jthread work;
};
