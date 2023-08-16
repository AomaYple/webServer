#include "Log.h"

#include <filesystem>

using namespace std;

Log::Log()
    : logFile{filesystem::current_path().string() + "/log.log", ofstream::trunc}, head{nullptr},
      work{&Log::loop, this} {}

auto Log::produce(string &&log) -> void {
    Node *const newHead{new Node{std::move(log), Log::instance.head.load(memory_order_relaxed)}};

    while (!Log::instance.head.compare_exchange_weak(newHead->next, newHead, memory_order_release,
                                                     memory_order_relaxed))
        ;

    Log::instance.notice.test_and_set(memory_order_relaxed);
    Log::instance.notice.notify_one();
}

[[noreturn]] auto Log::loop() -> void {
    while (true) {
        this->notice.wait(false, memory_order_relaxed);
        this->notice.clear(memory_order_relaxed);

        Node *pointer{this->head.load(memory_order_relaxed)};

        while (!this->head.compare_exchange_weak(pointer, nullptr, memory_order_release, memory_order_relaxed))
            ;

        Log::consume(Log::invertLinkedList(pointer));
    }
}

auto Log::invertLinkedList(Node *pointer) noexcept -> Node * {
    Node *previous{nullptr};

    while (pointer != nullptr) {
        Node *const next{pointer->next};

        pointer->next = previous;
        previous = pointer;

        pointer = next;
    }

    return previous;
}

auto Log::consume(Node *pointer) -> void {
    while (pointer != nullptr) {
        this->logFile << pointer->data;

        const Node *const oldPointer{pointer};
        pointer = pointer->next;

        delete oldPointer;
    }
}

Log::~Log() {
    Log::consume(Log::invertLinkedList(this->head));

    delete this->head.load(memory_order_relaxed);
}

Log Log::instance;
