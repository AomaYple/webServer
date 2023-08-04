#include "Request.h"

#include <ranges>

using std::string_view;
using std::views::split;

Request::Request(string_view request) {
    constexpr string_view delimiter{"\r\n"};

    for (const auto &valueView: request | split(delimiter)) {
        string_view value{valueView};

        if (this->method.empty()) this->parseLine(value);
        else if (!value.empty())
            this->parseHeader(value);
        else
            this->body = value;
    }
}

auto Request::parseLine(string_view line) -> void {
    for (const auto &wordView: line | split(' ')) {
        string_view word{wordView};

        if (this->method.empty()) this->method = word;
        else if (this->url.empty())
            this->url = word.substr(1);
        else
            this->version = word;
    }
}

auto Request::parseHeader(string_view header) -> void {
    constexpr string_view delimiter{": "};

    string_view key, value;

    for (const auto &wordView: header | split(delimiter)) {
        string_view word{wordView};

        if (!key.empty()) key = word;
        else
            value = word;
    }

    this->headers.emplace(key, value);
}

auto Request::getMethod() const noexcept -> string_view { return this->method; }

auto Request::getUrl() const noexcept -> string_view { return this->url; }

auto Request::getVersion() const noexcept -> string_view { return this->version; }

auto Request::getBody() const noexcept -> string_view { return this->body; }

auto Request::getHeaderValue(string_view filed) const -> string_view {
    auto result{this->headers.find(filed)};

    return result == this->headers.end() ? "" : result->second;
}
