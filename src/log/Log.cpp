#include "Log.h"

#include <filesystem>

using std::memory_order_relaxed, std::memory_order_release;
using std::ofstream, std::filesystem::current_path;
using std::source_location;
using std::string;
using std::chrono::system_clock;
using std::this_thread::get_id;

Log Log::instance;

auto Log::produce(source_location sourceLocation, Level level, string &&information) -> void {
    Node *newHead{new Node{Message{system_clock::now(), get_id(), sourceLocation, level, std::move(information)},
                           Log::instance.head.load(memory_order_relaxed)}};

    while (!Log::instance.head.compare_exchange_weak(newHead->next, newHead, memory_order_release,
                                                     memory_order_relaxed))
        ;

    Log::instance.notice.test_and_set(memory_order_relaxed);
    Log::instance.notice.notify_one();
}

auto Log::produce(Message &&message) -> void {
    Node *newHead{new Node{std::move(message), Log::instance.head.load(memory_order_relaxed)}};

    while (!Log::instance.head.compare_exchange_weak(newHead->next, newHead, memory_order_release,
                                                     memory_order_relaxed))
        ;

    Log::instance.notice.test_and_set(memory_order_relaxed);
    Log::instance.notice.notify_one();
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
        Message &message{pointer->message};

        this->logFile << message.combine();

        Node *oldPointer{pointer};
        pointer = pointer->next;
        delete oldPointer;
    }
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

Log::Log() : logFile{current_path().string() + "/log.log", ofstream::trunc}, head{nullptr}, work{&Log::loop, this} {}

Log::~Log() {
    try {
        Log::consume(Log::invertLinkedList(this->head));
    } catch (...) {
        Message message{system_clock::now(), get_id(), source_location::current(), Level::FATAL,
                        "log destructor error"};

        this->logFile << message.combine();
    }

    delete this->head.load(memory_order_relaxed);
}
