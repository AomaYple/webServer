#include "LogQueue.hpp"

LogQueue::LogQueue(LogQueue &&other) noexcept : head{other.head.load(std::memory_order_relaxed)} {
    other.head.store(nullptr, std::memory_order_relaxed);
}

auto LogQueue::operator=(LogQueue &&other) noexcept -> LogQueue & {
    if (this != &other) {
        LogQueue::traverse(this->head.load(std::memory_order_relaxed));

        this->head.store(other.head.load(std::memory_order_relaxed), std::memory_order_relaxed);

        other.head.store(nullptr, std::memory_order_relaxed);
    }

    return *this;
}

LogQueue::~LogQueue() { this->destroy(); }

auto LogQueue::push(Log &&log) noexcept -> void {
    Node *const newHead{new Node{std::move(log), this->head.load(std::memory_order_relaxed)}};

    while (!this->head.compare_exchange_weak(newHead->next, newHead, std::memory_order_release,
                                             std::memory_order_relaxed))
        ;
}

auto LogQueue::popAll() noexcept -> std::vector<Log> {
    Node *node{this->head.load(std::memory_order_relaxed)};

    while (!this->head.compare_exchange_weak(node, nullptr, std::memory_order_release, std::memory_order_relaxed))
        ;

    return LogQueue::traverse(LogQueue::invert(node));
}

auto LogQueue::copy() const noexcept -> LogQueue::Node * {
    Node *node{this->head.load(std::memory_order_relaxed)}, newNode{new Node{*node}};

    return newHead;
}

auto LogQueue::destroy() noexcept -> void {
    Node *node{this->head.load(std::memory_order_relaxed)};

    while (node != nullptr) {
        const Node *const oldNode{node};
        node = node->next;

        delete oldNode;
    }

    this->head.store(nullptr, std::memory_order_relaxed);
}

auto LogQueue::invert(Node *node) noexcept -> LogQueue::Node * {
    Node *previous{nullptr};

    while (node != nullptr) {
        Node *const next{node->next};

        node->next = previous;
        previous = node;

        node = next;
    }

    return previous;
}

auto LogQueue::traverse(Node *node) -> std::vector<Log> {
    std::vector<Log> logs;

    while (node != nullptr) {
        logs.emplace_back(std::move(node->log));

        const Node *const oldNode{node};
        node = node->next;

        delete oldNode;
    }

    return logs;
}
