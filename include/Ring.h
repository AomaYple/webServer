#pragma once

#include <liburing.h>

#include <functional>

class Completion;

class Ring {
public:
    Ring();

    Ring(const Ring &other) = delete;

    Ring(Ring &&other) noexcept;

    auto operator=(Ring &&other) noexcept -> Ring &;

    auto forEach(const std::function<auto(const Completion &)->void> &task) -> int;

    auto get() -> io_uring &;

    ~Ring();

private:
    auto registerCpu() -> void;

    auto registerMaxWorks() -> void;

    auto registerFileDescriptor() -> void;

    auto registerFileDescriptors() -> void;

    auto allocFileDescriptorRange(unsigned int start, unsigned int length) -> void;

    auto submitWait() -> void;

    static std::mutex lock;
    static std::vector<int> values;

    static thread_local bool instance;

    io_uring self;
};
