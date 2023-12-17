#pragma once

#include "JsonArray.hpp"
#include "JsonObject.hpp"

#include <variant>

class JsonValue {
public:
    enum class Type : unsigned char { null, boolean, number, string, array, object };

    explicit JsonValue(std::nullptr_t value) noexcept;

    explicit JsonValue(bool value) noexcept;

    explicit JsonValue(double value) noexcept;

    explicit JsonValue(std::string &&value) noexcept;

    explicit JsonValue(JsonArray &&value) noexcept;

    explicit JsonValue(JsonObject &&value) noexcept;

    [[nodiscard]] auto getType() const noexcept -> Type;

    [[nodiscard]] auto getNull() const noexcept -> std::nullptr_t;

    [[nodiscard]] auto getBool() const -> bool;

    [[nodiscard]] auto getBool() -> bool &;

    [[nodiscard]] auto getNumber() const -> double;

    [[nodiscard]] auto getNumber() -> double &;

    [[nodiscard]] auto getString() const -> std::string_view;

    [[nodiscard]] auto getString() -> std::string &;

    [[nodiscard]] auto getArray() const -> const JsonArray &;

    [[nodiscard]] auto getArray() -> JsonArray &;

    [[nodiscard]] auto getObject() const -> const JsonObject &;

    [[nodiscard]] auto getObject() -> JsonObject &;

    [[nodiscard]] auto toString() const -> std::string;

    [[nodiscard]] auto stringSize() const -> unsigned long;

private:
    [[nodiscard]] auto numberToString() const -> std::string;

    Type type;
    std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> value;
};
