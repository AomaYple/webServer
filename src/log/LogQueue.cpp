#include "LogQueue.hpp"

LogQueue::LogQueue(LogQueue &&other) noexcept : head{other.head.exchange(nullptr, std::memory_order::relaxed)} {}

auto LogQueue::operator=(LogQueue &&other) noexcept -> LogQueue & {
    if (this == &other) return *this;

    this->clear();
    this->head.store(other.head.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);

    return *this;
}

LogQueue::~LogQueue() { this->clear(); }

auto LogQueue::push(Log &&log) -> void {
    Node *const newHead{
        new Node{std::move(log), this->head.load(std::memory_order::relaxed)}
    };

    while (!this->head.compare_exchange_weak(newHead->next, newHead, std::memory_order::relaxed,
                                             std::memory_order::relaxed))
        ;
}

auto LogQueue::popAll() -> std::vector<Log> {
    std::vector<Log> logs;

    for (Node *node{this->invert()}; node != nullptr;) {
        logs.emplace_back(std::move(node->log));

        const Node *const oldNode{node};
        node = node->next;
        delete oldNode;
    }

    return logs;
}

auto LogQueue::clear() noexcept -> void {
    Node *node{this->head.exchange(nullptr, std::memory_order::relaxed)};

    while (node != nullptr) {
        const Node *const oldNode{node};
        node = node->next;
        delete oldNode;
    }
}

auto LogQueue::invert() noexcept -> Node * {
    Node *node{this->head.exchange(nullptr, std::memory_order::relaxed)}, *previous{};

    while (node != nullptr) {
        Node *const next{node->next};
        node->next = previous;
        previous = node;
        node = next;
    }

    return previous;
}
