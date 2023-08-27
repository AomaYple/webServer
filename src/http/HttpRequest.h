#pragma once

#include <string_view>
#include <unordered_map>

class HttpRequest {
public:
    static auto parse(std::string_view request) -> HttpRequest;

private:
    static auto parseLine(std::string_view line) -> std::array<std::string_view, 3>;

    static auto parseHeader(std::string_view header) -> std::pair<std::string_view, std::string_view>;

    HttpRequest(std::string_view method, std::string_view url, std::string_view version, std::string_view body,
                std::unordered_map<std::string_view, std::string_view> &&header);

public:
    auto getVersion() const noexcept -> std::string_view;

    auto getMethod() const noexcept -> std::string_view;

    auto getUrl() const noexcept -> std::string_view;

    auto getHeaderValue(std::string_view filed) const -> std::string_view;

    auto getBody() const noexcept -> std::string_view;

private:
    const std::string_view method, url, version, body;
    std::unordered_map<std::string_view, std::string_view> headers;
};
