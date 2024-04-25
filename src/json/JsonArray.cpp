#include "JsonArray.hpp"

#include "JsonValue.hpp"

JsonArray::JsonArray(std::string_view json) {
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
                    auto nextPoint{json.cbegin() + json.find('"', point - json.cbegin() + 1)};
                    this->values.emplace_back(std::string{point + 1, nextPoint});
                    point = nextPoint + 1;

                    break;
                }
            case '[':
                {
                    JsonArray array{json.substr(point - json.cbegin())};
                    point += array.stringSize();
                    this->values.emplace_back(std::move(array));

                    break;
                }
            case '{':
                {
                    JsonObject object{json.substr(point - json.cbegin())};
                    point += object.stringSize();
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

auto JsonArray::add(JsonValue &&value) -> void { this->values.emplace_back(std::move(value)); }

auto JsonArray::operator[](unsigned long index) const noexcept -> const JsonValue & { return this->values[index]; }

auto JsonArray::operator[](unsigned long index) noexcept -> JsonValue & { return this->values[index]; }

auto JsonArray::remove(long index) -> void { this->values.erase(this->values.cbegin() + index); }

auto JsonArray::toString() const -> std::string {
    std::string result{'['};
    for (const JsonValue &value : this->values) result += value.toString() + ',';

    if (result.back() == ',') result.pop_back();
    result += ']';

    return result;
}

auto JsonArray::stringSize() const -> unsigned long {
    unsigned long size{2};
    if (this->values.size() > 1) size += this->values.size() - 1;

    for (const JsonValue &value : this->values) size += value.stringSize();

    return size;
}
