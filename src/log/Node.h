#pragma once

#include <string>

struct Node {
    Node(std::string &&data, Node *next) noexcept;

    Node(const Node &) = delete;

    Node(Node &&) noexcept = default;

    std::string data;
    Node *next;
};
