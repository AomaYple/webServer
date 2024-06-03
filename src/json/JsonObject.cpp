#include "JsonObject.hpp"

#include "JsonValue.hpp"

JsonObject::JsonObject(const std::string_view json) {
    if (json.empty()) return;

    for (auto point{json.cbegin() + 1}; *point != '}';) {
        if (*point == ',') ++point;

        auto nextPoint{json.cbegin() + json.find('"', point - json.cbegin() + 1)};
        const std::string_view key{point + 1, nextPoint};

        point = nextPoint + 2;
        switch (*point) {
            case 'n':
                this->values.emplace(key, nullptr);
                point += 4;

                break;
            case 't':
                this->values.emplace(key, true);
                point += 4;

                break;
            case 'f':
                this->values.emplace(key, false);
                point += 5;

                break;
            case '"':
                nextPoint = json.cbegin() + json.find('"', point - json.cbegin() + 1);
                this->values.emplace(key, std::string{point + 1, nextPoint});
                point = nextPoint + 1;

                break;
            case '[':
                {
                    JsonArray array{json.substr(point - json.cbegin())};
                    point += array.stringSize();
                    this->values.emplace(key, std::move(array));

                    break;
                }
            case '{':
                {
                    JsonObject object{json.substr(point - json.cbegin())};
                    point += object.stringSize();
                    this->values.emplace(key, std::move(object));

                    break;
                }
            default:
                nextPoint = point + 1;
                while (*nextPoint != ',' && *nextPoint != '}') ++nextPoint;

                this->values.emplace(key, std::stod(std::string{point, nextPoint}));
                point = nextPoint;

                break;
        }
    }
}

auto JsonObject::add(const std::string_view key, JsonValue &&value) -> void {
    this->values.emplace(key, std::move(value));
}

auto JsonObject::operator[](const std::string &key) const -> const JsonValue & { return this->values.at(key); }

auto JsonObject::operator[](const std::string &key) -> JsonValue & { return this->values.at(key); }

auto JsonObject::remove(const std::string &key) -> void { this->values.erase(key); }

auto JsonObject::toString() const -> std::string {
    std::string result{'{'};
    for (const auto &[key, value] : this->values) result += '"' + key + "\":" + value.toString() + ',';

    if (result.back() == ',') result.pop_back();
    result += '}';

    return result;
}

auto JsonObject::stringSize() const -> unsigned long {
    unsigned long size{2};
    if (this->values.size() > 1) size += this->values.size() - 1;

    for (const auto &[key, value] : this->values) size += key.size() + 3 + value.stringSize();

    return size;
}
