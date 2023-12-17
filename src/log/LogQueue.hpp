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

    LogQueue(const LogQueue &);

    auto operator=(const LogQueue &) -> LogQueue &;

    LogQueue(LogQueue &&) noexcept;

    auto operator=(LogQueue &&) noexcept -> LogQueue &;

    ~LogQueue();

    auto push(Log &&log) -> void;

    [[nodiscard]] auto popAll() -> std::vector<Log>;

    auto clear() noexcept -> void;

private:
    [[nodiscard]] auto copy() const -> Node *;

    [[nodiscard]] auto invert() noexcept -> Node *;

    std::atomic<Node *> head;
};
