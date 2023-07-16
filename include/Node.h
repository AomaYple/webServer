#pragma once

#include "Data.h"

struct Node {
    Node(Data &&data, Node *next) noexcept;

    Node(const Node &) = delete;

    Node(Node &&) noexcept;

    auto operator=(Node &&) noexcept -> Node &;

    Data data;
    Node *next;
};
