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

    LogQueue(const LogQueue &) = delete;

    LogQueue(LogQueue &&other) noexcept;

    auto operator=(const LogQueue &) -> LogQueue & = delete;

    auto operator=(LogQueue &&other) noexcept -> LogQueue &;

    ~LogQueue();

    auto push(Log &&log) noexcept -> void;

    [[nodiscard]] auto popAll() noexcept -> std::vector<Log>;

private:
    [[nodiscard]] auto copy() const noexcept -> Node *;

    auto destroy() noexcept -> void;

    [[nodiscard]] static auto invert(Node *node) noexcept -> Node *;

    static auto traverse(Node *node) -> std::vector<Log>;

    std::atomic<Node *> head;
};
