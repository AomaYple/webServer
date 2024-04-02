#pragma once

#include "Log.hpp"

#include <vector>

class LogQueue {
    struct Node {
        Log log;
        Node *next{};
    };

public:
    constexpr LogQueue() noexcept = default;

    LogQueue(const LogQueue &) = delete;

    auto operator=(const LogQueue &) -> LogQueue & = delete;

    LogQueue(LogQueue &&) noexcept;

    auto operator=(LogQueue &&) noexcept -> LogQueue &;

    ~LogQueue();

    auto push(Log &&log) -> void;

    [[nodiscard]] auto popAll() -> std::vector<Log>;

    auto clear() noexcept -> void;

private:
    [[nodiscard]] auto invert() noexcept -> Node *;

    std::atomic<Node *> head;
};
