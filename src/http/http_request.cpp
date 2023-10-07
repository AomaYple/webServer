#include "http_request.hpp"

#include <ranges>

http_request::http_request(std::string_view request) {
    auto is_parse_line{false}, is_parse_body{false};

    constexpr std::string_view delimiter{"\r\n"};
    for (const auto &value_view: request | std::views::split(delimiter)) {
        const std::string_view value{value_view};

        if (!is_parse_line) {
            is_parse_line = true;

            this->parse_line(value);
        } else if (!value.empty() && !is_parse_body)
            this->parse_header(value);
        else if (is_parse_body)
            this->body = value;

        is_parse_body = value.empty();
    }
}

auto http_request::parse_line(std::string_view line) -> void {
    const auto point{line.cbegin() + line.find(' ')};
    this->method = {line.cbegin(), point};

    const auto next_point{line.cbegin() + line.find(' ', point - line.cbegin() + 2)};
    this->url = {point + 2, next_point};

    this->version = {next_point + 6, line.cend()};
}

auto http_request::parse_header(std::string_view header) -> void {
    const auto point{header.cbegin() + header.find(": ")};
    const std::string_view key{header.cbegin(), point}, value{point + 2, header.cend()};

    this->headers.emplace(key, value);
}

auto http_request::get_version() const noexcept -> std::string_view { return this->version; }

auto http_request::get_method() const noexcept -> std::string_view { return this->method; }

auto http_request::get_url() const noexcept -> std::string_view { return this->url; }

auto http_request::get_header_value(std::string_view filed) const -> std::string_view {
    const auto result{this->headers.find(filed)};

    return result == this->headers.cend() ? std::string_view{} : result->second;
}

auto http_request::get_body() const noexcept -> std::string_view { return this->body; }
