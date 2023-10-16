#include "Logger.hpp"

#include <filesystem>

Logger::~Logger() { Logger::consume(Logger::invertLinkedList(this->head.load(std::memory_order_relaxed))); }

auto Logger::produce(Log &&log) -> void {
    Node *const newHead{new Node{std::move(log), Logger::instance.head.load(std::memory_order_relaxed)}};

    while (!Logger::instance.head.compare_exchange_weak(newHead->next, newHead, std::memory_order_release,
                                                        std::memory_order_relaxed))
        ;

    Logger::instance.notice.test_and_set(std::memory_order_relaxed);
    Logger::instance.notice.notify_one();
}

auto Logger::stop() noexcept -> void {
    Logger::instance.worker.request_stop();

    Logger::instance.notice.test_and_set(std::memory_order_relaxed);
    Logger::instance.notice.notify_one();
}

Logger::Logger()
    : logFile{std::filesystem::current_path().string() + "/log.log", std::ofstream::trunc},
      worker{&Logger::run, &Logger::instance} {}

auto Logger::run() -> void {
    while (!Logger::instance.worker.get_stop_token().stop_requested()) {
        this->notice.wait(false, std::memory_order_relaxed);
        this->notice.clear(std::memory_order_relaxed);

        Node *pointer{this->head.load(std::memory_order_relaxed)};

        while (!this->head.compare_exchange_weak(pointer, nullptr, std::memory_order_release,
                                                 std::memory_order_relaxed))
            ;

        Logger::consume(Logger::invertLinkedList(pointer));
    }
}

auto Logger::invertLinkedList(Node *pointer) noexcept -> const Node * {
    Node *previous{nullptr};

    while (pointer != nullptr) {
        Node *const next{pointer->next};

        pointer->next = previous;
        previous = pointer;

        pointer = next;
    }

    return previous;
}

auto Logger::consume(const Node *pointer) -> void {
    while (pointer != nullptr) {
        this->logFile << pointer->log.toString();

        const Node *const oldPointer{pointer};
        pointer = pointer->next;

        delete oldPointer;
    }
}

Logger Logger::instance;
