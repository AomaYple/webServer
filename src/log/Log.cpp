#include "Log.h"

#include <chrono>
#include <filesystem>

using namespace std;

Log::Node::Node(string_view log, Node *next) : log{log}, next{next} {}

Log::Log()
    : logFile{filesystem::current_path().string() + "/log.log", ofstream::trunc}, head{nullptr}, work{&Log::run, this} {
}

auto Log::run() -> void {
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
        this->logFile << pointer->log;
        this->logFile.flush();

        const Node *const oldPointer{pointer};
        pointer = pointer->next;

        delete oldPointer;
    }
}

auto Log::formatLog(Level level, chrono::system_clock::time_point timestamp, jthread::id jThreadId,
                    source_location sourceLocation, string_view text) -> string {
    constexpr array<string_view, 4> levels{"Info", "Warn", "Error", "Fatal"};

    ostringstream jThreadIdStream;
    jThreadIdStream << jThreadId;

    return format("{} {} {} {}:{}:{}:{} {}\n", levels[static_cast<unsigned char>(level)], timestamp,
                  jThreadIdStream.str(), sourceLocation.file_name(), sourceLocation.line(), sourceLocation.column(),
                  sourceLocation.function_name(), text);
}

auto Log::produce(string_view log) -> void {
    Node *const newHead{new Node{log, Log::instance.head.load(memory_order_relaxed)}};

    while (!Log::instance.head.compare_exchange_weak(newHead->next, newHead, memory_order_release,
                                                     memory_order_relaxed))
        ;

    Log::instance.notice.test_and_set(memory_order_relaxed);
    Log::instance.notice.notify_one();
}

Log::~Log() {
    Log::consume(Log::invertLinkedList(this->head));

    delete this->head.load(memory_order_relaxed);
}

Log Log::instance;
