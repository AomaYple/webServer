#pragma once

#include <string>
#include <unordered_map>

class JsonValue;

class JsonObject {
    friend class JsonValue;
    friend class JsonArray;
    
public:
    explicit JsonObject(std::string_view json = std::string_view{});

    auto add(std::string_view key, JsonValue &&value) -> void;

    [[nodiscard]] auto operator[](const std::string &key) const -> const JsonValue &;

    [[nodiscard]] auto operator[](const std::string &key) -> JsonValue &;

    auto remove(const std::string &key) -> void;

    [[nodiscard]] auto toString() const -> std::string;

private:
    [[nodiscard]] auto stringSize() const -> unsigned long;

    std::unordered_map<std::string, JsonValue> values;
};
