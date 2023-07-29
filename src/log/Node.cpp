#include "Node.h"

Node::Node(Message &&message, Node *next)

        noexcept
    : message{std::move(message)}, next{next} {}
