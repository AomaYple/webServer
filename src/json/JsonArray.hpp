#pragma once

#include <string>
#include <vector>

class JsonValue;

class JsonArray {
public:
    explicit JsonArray(std::string_view json = std::string_view{});

    [[nodiscard]] auto toString() const -> std::string;

    auto add(JsonValue &&value) -> void;

    [[nodiscard]] auto operator[](unsigned long index) const noexcept -> const JsonValue &;

    [[nodiscard]] auto operator[](unsigned long index) noexcept -> JsonValue &;

    auto remove(long index) -> void;

    [[nodiscard]] auto stringSize() const -> unsigned long;

private:
    std::vector<JsonValue> values;
};
