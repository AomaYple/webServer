#pragma once

#include <vector>
#include <string_view>

class Buffer {
public:
    explicit Buffer(unsigned int size = 1024);

    Buffer(const Buffer &buffer) = default;

    Buffer(Buffer &&buffer) noexcept;

    auto operator=(Buffer &&buffer) noexcept -> Buffer &;

    auto write(const std::string_view &data) -> void;

    auto read() -> std::string_view;

    auto writableData() -> std::pair<char *, unsigned int>;

    auto adjustWrite(unsigned int size) -> void;

    auto adjustRead(unsigned int size) -> void;

    [[nodiscard]] auto empty() const -> bool;
private:
    std::vector<char> self;
    unsigned int start, end;
};
