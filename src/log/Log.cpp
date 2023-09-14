#include "Log.hpp"

#include <chrono>
#include <filesystem>

Log::Node::Node(std::string &&log, Node *next) noexcept : log{std::move(log)}, next{next} {}

Log::Log()
    : logFile{std::filesystem::current_path().string() + "/log.log", std::ofstream::trunc}, head{nullptr},
      work{&Log::run, this} {}

auto Log::run() -> void {
    while (true) {
        this->notice.wait(false, std::memory_order_relaxed);
        this->notice.clear(std::memory_order_relaxed);

        Node *pointer{this->head.load(std::memory_order_relaxed)};

        while (!this->head.compare_exchange_weak(pointer, nullptr, std::memory_order_release,
                                                 std::memory_order_relaxed))
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

        const Node *const oldPointer{pointer};
        pointer = pointer->next;

        delete oldPointer;
    }
}

auto Log::formatLog(Level level, std::chrono::system_clock::time_point timestamp, std::jthread::id jThreadId,
                    std::source_location sourceLocation, std::string &&text) -> std::string {
    constexpr std::array<std::string_view, 4> levels{"Info", "Warn", "Error", "Fatal"};

    std::ostringstream jThreadIdStream;
    jThreadIdStream << jThreadId;

    return format("{} {} {} {}:{}:{}:{} {}\n", levels[static_cast<unsigned char>(level)], timestamp,
                  jThreadIdStream.str(), sourceLocation.file_name(), sourceLocation.line(), sourceLocation.column(),
                  sourceLocation.function_name(), std::move(text));
}

auto Log::produce(std::string &&log) -> void {
    Node *const newHead{new Node{std::move(log), Log::instance.head.load(std::memory_order_relaxed)}};

    while (!Log::instance.head.compare_exchange_weak(newHead->next, newHead, std::memory_order_release,
                                                     std::memory_order_relaxed))
        ;

    Log::instance.notice.test_and_set(std::memory_order_relaxed);
    Log::instance.notice.notify_one();
}

Log Log::instance;
