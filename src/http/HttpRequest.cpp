#include "HttpRequest.hpp"

#include <ranges>

HttpRequest::HttpRequest(std::string_view request) {
    bool isParseLine{false}, isParseBody{false};

    constexpr std::string_view delimiter{"\r\n"};
    for (const auto &valueView: request | std::views::split(delimiter)) {
        const std::string_view value{valueView};

        if (!isParseLine) {
            isParseLine = true;

            this->parseLine(value);
        } else if (!value.empty() && !isParseBody)
            this->parseHeader(value);
        else if (isParseBody)
            this->body = value;

        isParseBody = value.empty();
    }
}

auto HttpRequest::parseLine(std::string_view line) -> void {
    const auto point{line.cbegin() + line.find(' ')};
    this->method = {line.cbegin(), point};

    const auto nextPoint{line.cbegin() + line.find(' ', point - line.cbegin() + 2)};
    this->url = {point + 2, nextPoint};

    this->version = {nextPoint + 6, line.cend()};
}

auto HttpRequest::parseHeader(std::string_view header) -> void {
    const auto point{header.cbegin() + header.find(": ")};
    const std::string_view key{header.cbegin(), point}, value{point + 2, header.cend()};

    this->headers.emplace(key, value);
}

auto HttpRequest::getVersion() const noexcept -> std::string_view { return this->version; }

auto HttpRequest::getMethod() const noexcept -> std::string_view { return this->method; }

auto HttpRequest::getUrl() const noexcept -> std::string_view { return this->url; }

auto HttpRequest::getHeaderValue(std::string_view filed) const -> std::string_view {
    const auto result{this->headers.find(filed)};

    return result == this->headers.end() ? std::string_view{} : result->second;
}

auto HttpRequest::getBody() const noexcept -> std::string_view { return this->body; }
