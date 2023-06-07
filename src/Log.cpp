#include "Log.h"

#include <format>
#include <iomanip>
#include <iostream>

using std::cout, std::string, std::string_view, std::array, std::ostringstream, std::put_time, std::jthread, std::this_thread::get_id,
        std::atomic_ref, std::memory_order_relaxed, std::memory_order_acquire, std::memory_order_release,
        std::chrono::system_clock, std::source_location, std::format;

constexpr array<string_view, 2> levels{"INFO", "ERROR"};

auto Log::add(source_location sourceLocation, Level level, string &&information) -> void {
    if (!instance.stop) {
        Node *newNode{new Node{}}, *oldTail{instance.tail.load(memory_order_relaxed)};

        while (!instance.tail.compare_exchange_weak(oldTail, newNode, memory_order_release, memory_order_relaxed));

        oldTail->data = Node::Message(system_clock::now(), get_id(), sourceLocation, level, std::move(information));
        oldTail->next = newNode;

        instance.notice.test_and_set(memory_order_release);
        instance.notice.notify_one();
    }
}

auto Log::stopWork() -> void {
    atomic_ref<bool> atomicStop{instance.stop};
    atomicStop = true;
}

Log::Log()
        : head{new Node{}}, stop{false}, work{[this] {
    while (!this->stop) {
        this->notice.wait(false, memory_order_acquire);
        this->notice.clear(memory_order_release);

        while (this->head != this->tail.load(memory_order_relaxed)) {
            auto &item{this->head->data};

            ostringstream stream;
            time_t now{system_clock::to_time_t(item.time)};
            stream << put_time(localtime(&now), "%F %T ") << " " << item.threadId << " ";
            cout << stream.str();

            cout << format("{}:{}:{}:{} {} {}\n", item.sourceLocation.file_name(), item.sourceLocation.line(),
                           item.sourceLocation.column(), item.sourceLocation.function_name(),
                           levels[static_cast<int>(item.level)], item.information);

            Node *oldHead{this->head};
            this->head = this->head->next;
            delete oldHead;
        }
    }
}} {
    this->head->next = new Node{};
    this->tail.store(this->head->next, memory_order_release);
    this->head = this->head->next;
}

Log::Node::Message::Message()
        : time{system_clock::now()}, threadId{get_id()}, sourceLocation{source_location::current()},
          level{Level::INFO} {}

Log::Node::Message::Message(system_clock::time_point time, jthread::id threadId, source_location sourceLocation,
                            Level level, string &&information)
        : time{time},
          threadId{threadId},
          sourceLocation{sourceLocation},
          level{level},
          information{std::move(information)} {}

Log::Node::Message::Message(Message &&message) noexcept
        : time{message.time},
          threadId{message.threadId},
          sourceLocation{message.sourceLocation},
          level{message.level},
          information{std::move(message.information)} {}

auto Log::Node::Message::operator=(Message &&message) noexcept -> Message & {
    if (this != &message) {
        this->time = message.time;
        this->threadId = message.threadId;
        this->sourceLocation = message.sourceLocation;
        this->level = message.level;
        this->information = std::move(message.information);
    }
    return *this;
}

Log::Node::Node() : next{nullptr} {}

Log::Node::Node(Message &&data, Node *next) : data{std::move(data)}, next{next} {}

Log::Node::Node(Node &&node) noexcept: data{std::move(node.data)}, next{node.next} {}

auto Log::Node::operator=(Node &&node) noexcept -> Node & {
    if (this != &node) {
        this->data = std::move(node.data);
        this->next = node.next;
        node.next = nullptr;
    }
    return *this;
}

Log Log::instance;
