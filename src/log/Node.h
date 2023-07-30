#pragma once

#include "Message.h"

struct Node {
    Node(Message &&message, Node *next) noexcept;

    Node(const Node &) = delete;

    Message message;
    Node *next;
};
