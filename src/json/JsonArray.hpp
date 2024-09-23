#pragma once

#include <string>
#include <vector>

class JsonValue;

class JsonArray {
    friend class JsonValue;
    friend class JsonObject;

public:
    explicit JsonArray(std::string_view json = {});

    auto pushBack(JsonValue &&value) -> void;

    [[nodiscard]] auto operator[](unsigned long index) const noexcept -> const JsonValue &;

    [[nodiscard]] auto operator[](unsigned long index) noexcept -> JsonValue &;

    auto erase(long index) -> void;

    [[nodiscard]] auto toString() const -> std::string;

private:
    [[nodiscard]] auto toStringSize() const -> unsigned long;

    std::vector<JsonValue> values;
};
