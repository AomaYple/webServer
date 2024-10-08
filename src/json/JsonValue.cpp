#include "JsonValue.hpp"

JsonValue::JsonValue(const std::nullptr_t value) noexcept : type{Type::null}, value{value} {}

JsonValue::JsonValue(const bool value) noexcept : type{Type::boolean}, value{value} {}

JsonValue::JsonValue(const double value) noexcept : type{Type::number}, value{value} {}

JsonValue::JsonValue(std::string &&value) noexcept : type{Type::string}, value{std::move(value)} {}

JsonValue::JsonValue(JsonArray &&value) noexcept : type{Type::array}, value{std::move(value)} {}

JsonValue::JsonValue(JsonObject &&value) noexcept : type{Type::object}, value{std::move(value)} {}

auto JsonValue::getType() const noexcept -> Type { return this->type; }

JsonValue::operator std::nullptr_t() const noexcept { return nullptr; }

JsonValue::operator bool() const { return std::get<bool>(this->value); }

JsonValue::operator double() const { return std::get<double>(this->value); }

JsonValue::operator std::string_view() const { return std::get<std::string>(this->value); }

JsonValue::operator std::string &() { return std::get<std::string>(this->value); }

JsonValue::operator const JsonArray &() const { return std::get<JsonArray>(this->value); }

JsonValue::operator JsonArray &() { return std::get<JsonArray>(this->value); }

JsonValue::operator const JsonObject &() const { return std::get<JsonObject>(this->value); }

JsonValue::operator JsonObject &() { return std::get<JsonObject>(this->value); }

auto JsonValue::operator=(const std::nullptr_t value) noexcept -> JsonValue & {
    this->type = Type::null;
    this->value = value;

    return *this;
}

auto JsonValue::operator=(const bool value) noexcept -> JsonValue & {
    this->type = Type::boolean;
    this->value = value;

    return *this;
}

auto JsonValue::operator=(const double value) noexcept -> JsonValue & {
    this->type = Type::number;
    this->value = value;

    return *this;
}

auto JsonValue::operator=(std::string &&value) noexcept -> JsonValue & {
    this->type = Type::string;
    this->value = std::move(value);

    return *this;
}

auto JsonValue::operator=(JsonArray &&value) noexcept -> JsonValue & {
    this->type = Type::array;
    this->value = std::move(value);

    return *this;
}

auto JsonValue::operator=(JsonObject &&value) noexcept -> JsonValue & {
    this->type = Type::object;
    this->value = std::move(value);

    return *this;
}

auto JsonValue::toString() const -> std::string {
    switch (this->type) {
        case Type::null:
            return "null";
        case Type::boolean:
            return std::get<bool>(this->value) ? "true" : "false";
        case Type::number:
            return this->numberToString();
        case Type::string:
            return '"' + std::get<std::string>(this->value) + '"';
        case Type::array:
            return std::get<JsonArray>(this->value).toString();
        case Type::object:
            return std::get<JsonObject>(this->value).toString();
    }

    return "";
}

auto JsonValue::toStringSize() const -> unsigned long {
    switch (this->type) {
        case Type::null:
            return 4;
        case Type::boolean:
            return std::get<bool>(this->value) ? 4 : 5;
        case Type::number:
            return this->numberToString().size();
        case Type::string:
            return std::get<std::string>(this->value).size() + 2;
        case Type::array:
            return std::get<JsonArray>(this->value).toStringSize();
        case Type::object:
            return std::get<JsonObject>(this->value).toStringSize();
    }

    return 0;
}

auto JsonValue::numberToString() const -> std::string {
    auto result{std::to_string(std::get<double>(this->value))};
    result.erase(result.find_last_not_of('0') + 1, std::string::npos);
    result.erase(result.find_last_not_of('.') + 1, std::string::npos);

    return result;
}
