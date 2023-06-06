#pragma once

#include <memory>

#include "Ring.h"

class Buffer {
public:
    explicit Buffer(std::shared_ptr<Ring> &ioUserRing);

    Buffer(const Buffer &buffer) = delete;

    Buffer(Buffer &&buffer) noexcept;

    auto operator=(Buffer &&buffer) noexcept -> Buffer &;

    [[nodiscard]] auto getId() const -> unsigned short;

    auto getData(unsigned short index, unsigned long size) -> std::string;

    auto advanceBufferCompletion(int number) -> void;

    ~Buffer();

private:
    static thread_local bool instance;

    io_uring_buf_ring *self;
    std::vector<std::vector<char>> buffers;
    unsigned short id, offset;
    int mask;
    std::shared_ptr<Ring> ring;
};
