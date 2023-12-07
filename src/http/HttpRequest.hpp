#pragma once

#include <string_view>
#include <unordered_map>

class HttpRequest {
public:
    explicit HttpRequest(std::string_view request) noexcept;

    [[nodiscard]] auto getVersion() const noexcept -> std::string_view;

    [[nodiscard]] auto getMethod() const noexcept -> std::string_view;

    [[nodiscard]] auto getUrl() const noexcept -> std::string_view;

    [[nodiscard]] auto getHeaderValue(std::string_view filed) const noexcept -> std::string_view;

    [[nodiscard]] auto getBody() const noexcept -> std::string_view;

private:
    auto parseLine(std::string_view line) noexcept -> void;

    auto parseHeader(std::string_view header) noexcept -> void;

    std::string_view method, url, version, body;
    std::unordered_map<std::string_view, const std::string_view> headers;
};
