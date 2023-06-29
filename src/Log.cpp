#include "Log.h"

#include <chrono>
#include <format>
#include <iostream>

using std::cout, std::string, std::string_view, std::array, std::this_thread::get_id, std::memory_order_relaxed,
        std::memory_order_release, std::chrono::system_clock, std::source_location, std::format;

constexpr array<string_view, 3> levels{"INFO", "WARN", "ERROR"};

Log Log::instance;

auto Log::produce(source_location sourceLocation, Level level, string &&information) noexcept -> void {
    Node *newHead{new Node{Data{system_clock::now(), get_id(), sourceLocation, level, std::move(information)},
                           instance.head.load(memory_order_relaxed)}};

    while (!instance.head.compare_exchange_weak(newHead->next, newHead, memory_order_release, memory_order_relaxed))
        ;

    instance.notice.test_and_set(memory_order_relaxed);
    instance.notice.notify_one();
}

auto Log::invertLinkedList(Node *pointer) noexcept -> Node * {
    Node *previous{nullptr};

    while (pointer != nullptr) {
        Node *next{pointer->next};

        pointer->next = previous;
        previous = pointer;

        pointer = next;
    }

    return previous;
}

auto Log::consume(Node *pointer) -> void {
    while (pointer != nullptr) {
        Data &data{pointer->data};

        cout << data.timestamp << " " << data.threadId << " "
             << format("{}:{}:{}:{} {} {}\n", data.sourceLocation.file_name(), data.sourceLocation.line(),
                       data.sourceLocation.column(), data.sourceLocation.function_name(),
                       levels[static_cast<int>(data.level)], data.information);

        Node *oldPointer{pointer};
        pointer = pointer->next;
        delete oldPointer;
    }
}

Log::Log()
    : head{nullptr}, work{[this] {
          while (true) {
              this->notice.wait(false, memory_order_relaxed);
              this->notice.clear(memory_order_relaxed);

              Node *pointer{this->head.load(memory_order_relaxed)};

              while (!this->head.compare_exchange_weak(pointer, nullptr, memory_order_release, memory_order_relaxed))
                  ;

              Log::consume(Log::invertLinkedList(pointer));
          }
      }} {}

Log::~Log() { Log::consume(Log::invertLinkedList(this->head)); }
