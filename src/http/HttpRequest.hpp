#pragma once

#include <string_view>
#include <unordered_map>

class HttpRequest {
public:
    explicit HttpRequest(std::string_view request);

    [[nodiscard]] auto getVersion() const noexcept -> std::string_view;

    [[nodiscard]] auto getMethod() const noexcept -> std::string_view;

    [[nodiscard]] auto getUrl() const noexcept -> std::string_view;

    [[nodiscard]] auto containsHeader(std::string_view filed) const -> bool;

    [[nodiscard]] auto getHeaderValue(std::string_view filed) const -> std::string_view;

    [[nodiscard]] auto getBody() const noexcept -> std::string_view;

private:
    auto parseLine(std::string_view line) noexcept -> void;

    auto parseHeader(std::string_view header) -> void;

    std::string_view method, url, version, body;
    std::unordered_map<std::string_view, const std::string_view> headers;
};
