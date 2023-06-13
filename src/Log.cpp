#include "Log.h"

#include <format>
#include <iomanip>
#include <iostream>

using std::cout, std::string, std::string_view, std::array, std::ostringstream, std::put_time, std::jthread,
    std::this_thread::get_id, std::atomic_ref, std::memory_order_relaxed, std::memory_order_acquire,
    std::memory_order_release, std::chrono::system_clock, std::source_location, std::format;

constexpr array<string_view, 3> levels{"INFO", "WARN", "ERROR"};

auto Log::add(source_location sourceLocation, Level level, string &&message) -> void {
    if (!instance.stop) {
        Node *newNode{new Node{}}, *oldTail{instance.tail.load(memory_order_relaxed)};

        while (!instance.tail.compare_exchange_weak(oldTail, newNode, memory_order_release, memory_order_relaxed))
            ;

        oldTail->data = Node::Data(system_clock::now(), get_id(), sourceLocation, level, std::move(message));
        oldTail->next = newNode;

        instance.notice.test_and_set(memory_order_release);
        instance.notice.notify_one();
    }
}

auto Log::stopWork() -> void {
    atomic_ref<bool> atomicStop{instance.stop};
    atomicStop.store(true, memory_order_release);
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
                                 levels[static_cast<int>(item.level)], item.message);

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

Log::Node::Data::Data()
    : time{system_clock::now()}, threadId{get_id()}, sourceLocation{source_location::current()}, level{Level::INFO} {}

Log::Node::Data::Data(system_clock::time_point time, jthread::id threadId, source_location sourceLocation, Level level,
                      string &&message)
    : time{time}, threadId{threadId}, sourceLocation{sourceLocation}, level{level}, message{std::move(message)} {}

Log::Node::Data::Data(Data &&other) noexcept
    : time{other.time},
      threadId{other.threadId},
      sourceLocation{other.sourceLocation},
      level{other.level},
      message{std::move(other.message)} {}

auto Log::Node::Data::operator=(Data &&other) noexcept -> Data & {
    if (this != &other) {
        this->time = other.time;
        this->threadId = other.threadId;
        this->sourceLocation = other.sourceLocation;
        this->level = other.level;
        this->message = std::move(other.message);
    }
    return *this;
}

Log::Node::Node() : next{nullptr} {}

Log::Node::Node(Data &&data, Node *next) : data{std::move(data)}, next{next} {}

Log::Node::Node(Node &&other) noexcept : data{std::move(other.data)}, next{other.next} {}

auto Log::Node::operator=(Node &&other) noexcept -> Node & {
    if (this != &other) {
        this->data = std::move(other.data);
        this->next = other.next;
    }
    return *this;
}

Log Log::instance;
