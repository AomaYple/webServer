#pragma once

#include "Log.hpp"

struct Node {
    Node(Log &&log, Node *next) noexcept;

    Node(const Node &) = delete;

    Node(Node &&) = default;

    auto operator=(const Node &) -> Node & = delete;

    auto operator=(Node &&) -> Node & = delete;

    ~Node() noexcept = default;

    Log log;
    Node *next;
};
