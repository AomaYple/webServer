#pragma once

#include "Log.hpp"

#include <vector>

class LogQueue {
    struct Node {
        Log log;
        Node *next{};
    };

public:
    LogQueue() noexcept = default;

    LogQueue(const LogQueue &) noexcept;

    auto operator=(const LogQueue &) noexcept -> LogQueue &;

    LogQueue(LogQueue &&) noexcept;

    auto operator=(LogQueue &&) noexcept -> LogQueue &;

    ~LogQueue();

    auto push(Log &&log) noexcept -> void;

    [[nodiscard]] auto popAll() noexcept -> std::vector<Log>;

    auto clear() noexcept -> void;

private:
    [[nodiscard]] auto copy() const noexcept -> Node *;

    [[nodiscard]] auto invert() noexcept -> Node *;

    std::atomic<Node *> head;
};
