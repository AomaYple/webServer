#pragma once

#include <string_view>
#include <vector>

class Buffer {
public:
    Buffer();

    Buffer(const Buffer &other) = default;

    Buffer(Buffer &&other) noexcept;

    auto operator=(Buffer &&other) noexcept -> Buffer &;

    auto write(std::string_view data) -> void;

    auto read() -> std::string_view;

    auto writableData() -> std::pair<char *, unsigned int>;

    auto adjustWrite(unsigned int size) -> void;

    auto adjustRead(unsigned int size) -> void;

private:
    std::vector<char> self;
    unsigned int start, end;
};