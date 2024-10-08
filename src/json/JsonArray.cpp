#include "JsonArray.hpp"

#include "JsonValue.hpp"

JsonArray::JsonArray(const std::string_view json) {
    if (json.empty()) return;

    for (auto point{json.cbegin() + 1}; *point != ']';) {
        switch (*point) {
            case ',':
                ++point;

                break;
            case 'n':
                this->values.emplace_back(nullptr);
                point += 4;

                break;
            case 't':
                this->values.emplace_back(true);
                point += 4;

                break;
            case 'f':
                this->values.emplace_back(false);
                point += 5;

                break;
            case '"':
                {
                    const auto nextPoint{json.cbegin() + json.find('"', point - json.cbegin() + 1)};
                    this->values.emplace_back(std::string{point + 1, nextPoint});
                    point = nextPoint + 1;

                    break;
                }
            case '[':
                {
                    JsonArray array{json.substr(point - json.cbegin())};
                    point += array.toStringSize();
                    this->values.emplace_back(std::move(array));

                    break;
                }
            case '{':
                {
                    JsonObject object{json.substr(point - json.cbegin())};
                    point += object.toStringSize();
                    this->values.emplace_back(std::move(object));

                    break;
                }
            default:
                {
                    auto nextPoint{point + 1};
                    while (*nextPoint != ',' && *nextPoint != ']') ++nextPoint;

                    this->values.emplace_back(std::stod(std::string{point, nextPoint}));
                    point = nextPoint;

                    break;
                }
        }
    }
}

auto JsonArray::pushBack(JsonValue &&value) -> void { this->values.emplace_back(std::move(value)); }

auto JsonArray::operator[](const unsigned long index) const noexcept -> const JsonValue & {
    return this->values[index];
}

auto JsonArray::operator[](const unsigned long index) noexcept -> JsonValue & { return this->values[index]; }

auto JsonArray::erase(const long index) -> void { this->values.erase(this->values.cbegin() + index); }

auto JsonArray::toString() const -> std::string {
    std::string result{'['};
    for (const auto &value : this->values) result += value.toString() + ',';

    if (result.back() == ',') result.pop_back();
    result += ']';

    return result;
}

auto JsonArray::toStringSize() const -> unsigned long {
    unsigned long size{2};
    if (this->values.size() > 1) size += this->values.size() - 1;

    for (const auto &value : this->values) size += value.toStringSize();

    return size;
}
