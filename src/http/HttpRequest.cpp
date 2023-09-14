#include "HttpRequest.hpp"

#include <algorithm>
#include <ranges>

auto HttpRequest::parse(std::string_view request) -> HttpRequest {
    std::array<std::string_view, 4> result;
    std::unordered_map<std::string_view, std::string_view> header;

    bool isBody{false};

    constexpr std::string_view delimiter{"\r\n"};
    for (const auto &valueView: request | std::views::split(delimiter)) {
        const std::string_view value{valueView};

        if (result.begin()->empty()) std::ranges::copy(HttpRequest::parseLine(value), result.begin());
        else if (!value.empty() && !isBody)
            header.emplace(HttpRequest::parseHeader(value));
        else if (isBody)
            result[3] = value;

        isBody = value.empty();
    }

    return HttpRequest{result[0], result[1], result[2], result[3], std::move(header)};
}

auto HttpRequest::parseLine(std::string_view line) -> std::array<std::string_view, 3> {
    std::array<std::string_view, 3> result;

    for (auto point{result.begin()}; const auto &wordView: line | std::views::split(' '))
        *point++ = std::string_view{wordView};

    result[1] = result[1].substr(1);
    result[2] = result[2].substr(5);

    return result;
}

auto HttpRequest::parseHeader(std::string_view header) -> std::pair<std::string_view, std::string_view> {
    std::array<std::string_view, 2> result;

    constexpr std::string_view delimiter{": "};
    for (auto point{result.begin()}; const auto &wordView: header | std::views::split(delimiter))
        *point++ = std::string_view{wordView};

    return {result[0], result[1]};
}

HttpRequest::HttpRequest(std::string_view method, std::string_view url, std::string_view version, std::string_view body,
                         std::unordered_map<std::string_view, std::string_view> &&header)
    : method{method}, url{url}, version{version}, body{body}, headers{std::move(header)} {}

auto HttpRequest::getVersion() const noexcept -> std::string_view { return this->version; }

auto HttpRequest::getMethod() const noexcept -> std::string_view { return this->method; }

auto HttpRequest::getUrl() const noexcept -> std::string_view { return this->url; }

auto HttpRequest::getHeaderValue(std::string_view filed) const -> std::string_view {
    const auto result{this->headers.find(filed)};

    return result == this->headers.end() ? std::string_view{} : result->second;
}

auto HttpRequest::getBody() const noexcept -> std::string_view { return this->body; }
