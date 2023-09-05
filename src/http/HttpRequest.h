#pragma once

#include <string_view>
#include <unordered_map>

class HttpRequest {
public:
    [[nodiscard]] static auto parse(std::string_view request) noexcept -> HttpRequest;

private:
    [[nodiscard]] static auto parseLine(std::string_view line) noexcept -> std::array<std::string_view, 3>;

    [[nodiscard]] static auto parseHeader(std::string_view header) noexcept
            -> std::pair<std::string_view, std::string_view>;

    HttpRequest(std::string_view method, std::string_view url, std::string_view version, std::string_view body,
                std::unordered_map<std::string_view, std::string_view> &&header) noexcept;

public:
    [[nodiscard]] auto getVersion() const noexcept -> std::string_view;

    [[nodiscard]] auto getMethod() const noexcept -> std::string_view;

    [[nodiscard]] auto getUrl() const noexcept -> std::string_view;

    [[nodiscard]] auto getHeaderValue(std::string_view filed) const noexcept -> std::string_view;

    [[nodiscard]] auto getBody() const noexcept -> std::string_view;

private:
    const std::string_view method, url, version, body;
    std::unordered_map<std::string_view, std::string_view> headers;
};
