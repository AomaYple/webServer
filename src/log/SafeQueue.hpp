#pragma once

#include <atomic>
#include <vector>

template<typename T>
    requires std::movable<T>
class SafeQueue {
    struct Node {
        T data;
        Node *next{};
    };

public:
    constexpr SafeQueue() noexcept = default;

    SafeQueue(const SafeQueue &other) : head{other.copy()} {}

    auto operator=(const SafeQueue &other) -> SafeQueue & {
        this->clear();

        this->head.store(other.copy(), std::memory_order::relaxed);

        return *this;
    }

    SafeQueue(SafeQueue &&other) noexcept : head{other.head.exchange(nullptr, std::memory_order::relaxed)} {}

    auto operator=(SafeQueue &&other) noexcept -> SafeQueue & {
        this->clear();

        this->head.store(other.head.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);

        return *this;
    }

    ~SafeQueue() { this->clear(); }

    auto push(T &&data) -> void {
        Node *const newHead{new Node{std::move(data), this->head.load(std::memory_order::relaxed)}};

        while (!this->head.compare_exchange_weak(newHead->next, newHead, std::memory_order::release,
                                                 std::memory_order::relaxed))
            ;
    }

    [[nodiscard]] auto popAll() -> std::vector<T> {
        std::vector<T> result;

        for (Node *node{this->invert()}; node != nullptr;) {
            result.emplace_back(std::move(node->data));

            const Node *const oldNode{node};
            node = node->next;
            delete oldNode;
        }

        return result;
    }

    auto clear() noexcept -> void {
        Node *node{this->head.exchange(nullptr, std::memory_order::relaxed)};

        while (node != nullptr) {
            const Node *const oldNode{node};
            node = node->next;
            delete oldNode;
        }
    }

private:
    [[nodiscard]] auto copy() const -> Node * {
        Node *node{this->head.load(std::memory_order::relaxed)}, *newNode{};

        for (Node *current{newNode}; node != nullptr; node = node->next) {
            if (current == nullptr) current = newNode = new Node{node->data};
            else {
                current->next = new Node{node->data};
                current = current->next;
            }
        }

        return newNode;
    }

    [[nodiscard]] auto invert() noexcept -> Node * {
        Node *node{this->head.exchange(nullptr, std::memory_order::relaxed)}, *previous{};

        while (node != nullptr) {
            Node *const next{node->next};
            node->next = previous;
            previous = node;
            node = next;
        }

        return previous;
    }

    std::atomic<Node *> head;
};
