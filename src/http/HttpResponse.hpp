#pragma once

#include <span>
#include <string_view>
#include <vector>

class HttpResponse {
public:
    HttpResponse() = default;

    auto setVersion(std::string_view version) -> void;

    auto setStatusCode(std::string_view statusCode) -> void;

    auto addHeader(std::string_view header) -> void;

    auto clearHeaders() noexcept -> void;

    auto setBody(std::span<const std::byte> body) -> void;

    [[nodiscard]] auto toByte() const -> std::vector<std::byte>;

private:
    std::vector<std::byte> version, statusCode, headers, body;
};
