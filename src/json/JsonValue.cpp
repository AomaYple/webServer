#include "JsonValue.hpp"

JsonValue::JsonValue(std::nullptr_t value) noexcept : type{Type::null}, value{value} {}

JsonValue::JsonValue(bool value) noexcept : type{Type::boolean}, value{value} {}

JsonValue::JsonValue(double value) noexcept : type{Type::number}, value{value} {}

JsonValue::JsonValue(std::string &&value) noexcept : type{Type::string}, value{std::move(value)} {}

JsonValue::JsonValue(JsonArray &&value) noexcept : type{Type::array}, value{std::move(value)} {}

JsonValue::JsonValue(JsonObject &&value) noexcept : type{Type::object}, value{std::move(value)} {}

auto JsonValue::getType() const noexcept -> enum JsonValue::Type { return this->type; }

auto JsonValue::getNull() const noexcept -> std::nullptr_t { return std::get<std::nullptr_t>(this->value); }

auto JsonValue::getBool() const -> bool { return std::get<bool>(this->value); }

auto JsonValue::getBool() -> bool & { return std::get<bool>(this->value); }

auto JsonValue::getNumber() const -> double { return std::get<double>(this->value); }

auto JsonValue::getNumber() -> double & { return std::get<double>(this->value); }

auto JsonValue::getString() const -> std::string_view { return std::get<std::string>(this->value); }

auto JsonValue::getString() -> std::string & { return std::get<std::string>(this->value); }

auto JsonValue::getArray() const -> const JsonArray & { return std::get<JsonArray>(this->value); }

auto JsonValue::getArray() -> JsonArray & { return std::get<JsonArray>(this->value); }

auto JsonValue::getObject() const -> const JsonObject & { return std::get<JsonObject>(this->value); }

auto JsonValue::getObject() -> JsonObject & { return std::get<JsonObject>(this->value); }

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

auto JsonValue::stringSize() const -> unsigned long {
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
            return std::get<JsonArray>(this->value).stringSize();
        case Type::object:
            return std::get<JsonObject>(this->value).stringSize();
    }

    return 0;
}

auto JsonValue::numberToString() const -> std::string {
    std::string result{std::to_string(std::get<double>(this->value))};

    result.erase(result.find_last_not_of('0') + 1, std::string::npos);
    if (result.back() == '.') result.pop_back();

    return result;
}
