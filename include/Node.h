#pragma once

#include "Data.h"

struct Node {
    Node(Data &&data, Node *next) noexcept;

    Node(const Node &other) = delete;

    Node(Node &&other) noexcept;

    auto operator=(Node &&other) noexcept -> Node &;

    Data data;
    Node *next;
};
