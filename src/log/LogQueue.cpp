#include "LogQueue.hpp"

LogQueue::LogQueue(const LogQueue &other) noexcept : head{other.copy()} {}

auto LogQueue::operator=(const LogQueue &other) noexcept -> LogQueue & {
    if (this != &other) {
        this->clear();

        this->head.store(other.copy(), std::memory_order_relaxed);
    }

    return *this;
}

LogQueue::LogQueue(LogQueue &&other) noexcept : head{other.head.exchange(nullptr, std::memory_order_relaxed)} {}

auto LogQueue::operator=(LogQueue &&other) noexcept -> LogQueue & {
    if (this != &other) {
        this->clear();

        this->head.store(other.head.exchange(nullptr, std::memory_order_relaxed), std::memory_order_relaxed);
    }

    return *this;
}

LogQueue::~LogQueue() { this->clear(); }

auto LogQueue::push(Log &&log) noexcept -> void {
    Node *const newHead{new Node{std::move(log), this->head.load(std::memory_order_relaxed)}};

    while (!this->head.compare_exchange_weak(newHead->next, newHead, std::memory_order_release,
                                             std::memory_order_relaxed))
        ;
}

auto LogQueue::popAll() noexcept -> std::vector<Log> {
    std::vector<Log> logs;

    for (Node *node{this->invert()}; node;) {
        logs.emplace_back(std::move(node->log));

        const Node *const oldNode{node};
        node = node->next;
        delete oldNode;
    }

    return logs;
}

auto LogQueue::clear() noexcept -> void {
    Node *node{this->head.exchange(nullptr, std::memory_order_relaxed)};

    while (node != nullptr) {
        const Node *const oldNode{node};
        node = node->next;
        delete oldNode;
    }
}

auto LogQueue::copy() const noexcept -> LogQueue::Node * {
    Node *node{this->head.load(std::memory_order_relaxed)}, *otherNode{new Node};

    for (Node *current{otherNode}; node; node = node->next) {
        current->next = new Node{node->log};
        current = current->next;
    }

    const Node *const oldOtherNode{otherNode};
    otherNode = otherNode->next;
    delete oldOtherNode;

    return otherNode;
}

auto LogQueue::invert() noexcept -> Node * {
    Node *node{this->head.exchange(nullptr, std::memory_order_relaxed)}, *previous{};

    while (node) {
        Node *const next{node->next};

        node->next = previous;
        previous = node;
        node = next;
    }

    return previous;
}
