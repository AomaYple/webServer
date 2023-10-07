#pragma once

#include <string_view>
#include <unordered_map>

class http_request {
public:
    explicit http_request(std::string_view request);

private:
    auto parse_line(std::string_view line) -> void;

    auto parse_header(std::string_view header) -> void;

public:
    [[nodiscard]] auto get_version() const noexcept -> std::string_view;

    [[nodiscard]] auto get_method() const noexcept -> std::string_view;

    [[nodiscard]] auto get_url() const noexcept -> std::string_view;

    [[nodiscard]] auto get_header_value(std::string_view filed) const -> std::string_view;

    [[nodiscard]] auto get_body() const noexcept -> std::string_view;

private:
    std::string_view method, url, version, body;
    std::unordered_map<std::string_view, std::string_view> headers;
};
