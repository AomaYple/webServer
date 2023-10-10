#include "Node.hpp"

Node::Node(Log &&log, Node *next) noexcept : log{std::move(log)}, next{next} {}
