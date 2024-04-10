#pragma once

#include <string>
#include <vector>

class JsonValue;

class JsonArray {
    friend class JsonValue;
    friend class JsonObject;

public:
    explicit JsonArray(std::string_view json = {});

    auto add(JsonValue &&value) -> void;

    [[nodiscard]] auto operator[](unsigned long index) const noexcept -> const JsonValue &;

    [[nodiscard]] auto operator[](unsigned long index) noexcept -> JsonValue &;

    auto remove(long index) -> void;

    [[nodiscard]] auto toString() const -> std::string;

private:
    [[nodiscard]] auto stringSize() const -> unsigned long;

    std::vector<JsonValue> values;
};
