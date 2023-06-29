#include "Node.h"

Node::Node(Data &&data, Node *next) noexcept : data{std::move(data)}, next{next} {}

Node::Node(Node &&other) noexcept : data{std::move(other.data)}, next{other.next} {}

auto Node::operator=(Node &&other) noexcept -> Node & {
    if (this != &other) {
        this->data = std::move(other.data);
        this->next = other.next;
    }
    return *this;
}
