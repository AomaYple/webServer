#pragma once

#include "JsonArray.hpp"
#include "JsonObject.hpp"

#include <variant>

class JsonValue {
    friend class JsonArray;
    friend class JsonObject;

public:
    enum class Type : unsigned char { null, boolean, number, string, array, object };

    explicit JsonValue(std::nullptr_t value = {}) noexcept;

    explicit JsonValue(bool value) noexcept;

    explicit JsonValue(double value) noexcept;

    explicit JsonValue(std::string &&value) noexcept;

    explicit JsonValue(JsonArray &&value) noexcept;

    explicit JsonValue(JsonObject &&value) noexcept;

    [[nodiscard]] auto getType() const noexcept -> Type;

    explicit operator std::nullptr_t() const noexcept;

    explicit operator bool() const;

    explicit operator double() const;

    explicit operator std::string_view() const;

    explicit operator std::string &();

    explicit operator const JsonArray &() const;

    explicit operator JsonArray &();

    explicit operator const JsonObject &() const;

    explicit operator JsonObject &();

    auto operator=(std::nullptr_t value) noexcept -> JsonValue &;

    auto operator=(bool value) noexcept -> JsonValue &;

    auto operator=(double value) noexcept -> JsonValue &;

    auto operator=(std::string &&value) noexcept -> JsonValue &;

    auto operator=(JsonArray &&value) noexcept -> JsonValue &;

    auto operator=(JsonObject &&value) noexcept -> JsonValue &;

    [[nodiscard]] auto toString() const -> std::string;

private:
    [[nodiscard]] auto stringSize() const -> unsigned long;

    [[nodiscard]] auto numberToString() const -> std::string;

    Type type;
    std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject> value;
};
