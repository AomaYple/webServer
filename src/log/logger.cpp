#include "logger.hpp"

#include <filesystem>

logger::logger() : log_file{std::filesystem::current_path().string() + "/log.log", std::ofstream::trunc} {}

logger::~logger() { logger::consume(logger::invert_linked_list(this->head.load(std::memory_order_relaxed))); }

auto logger::start() -> void { logger::instance.worker = std::jthread{&logger::run, &logger::instance}; }

auto logger::stop() noexcept -> void { logger::instance.worker.request_stop(); }

auto logger::run(const std::stop_token &stop_token) -> void {
    while (!stop_token.stop_requested()) {
        this->notice.wait(false, std::memory_order_relaxed);
        this->notice.clear(std::memory_order_relaxed);

        node *pointer{this->head.load(std::memory_order_relaxed)};

        while (!this->head.compare_exchange_weak(pointer, nullptr, std::memory_order_release,
                                                 std::memory_order_relaxed))
            ;

        logger::consume(logger::invert_linked_list(pointer));
    }
}

auto logger::invert_linked_list(node *pointer) noexcept -> node * {
    node *previous{nullptr};

    while (pointer != nullptr) {
        node *const next{pointer->next};

        pointer->next = previous;
        previous = pointer;

        pointer = next;
    }

    return previous;
}

auto logger::consume(node *pointer) -> void {
    while (pointer != nullptr) {
        this->log_file << pointer->log.to_string();

        const node *const oldPointer{pointer};
        pointer = pointer->next;

        delete oldPointer;
    }
}

auto logger::produce(log &&log) -> void {
    node *const newHead{new node{std::move(log), logger::instance.head.load(std::memory_order_relaxed)}};

    while (!logger::instance.head.compare_exchange_weak(newHead->next, newHead, std::memory_order_release,
                                                        std::memory_order_relaxed))
        ;

    logger::instance.notice.test_and_set(std::memory_order_relaxed);
    logger::instance.notice.notify_one();
}

logger logger::instance;
