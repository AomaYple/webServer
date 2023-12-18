#pragma once

#include <span>
#include <string_view>
#include <vector>

class HttpResponse {
public:
    constexpr HttpResponse() noexcept = default;

    auto setVersion(std::string_view element) -> void;

    auto setStatusCode(std::string_view newStatusCode) -> void;

    auto addHeader(std::string_view header) -> void;

    auto clearHeaders() noexcept -> void;

    auto setBody(std::span<const std::byte> newBody) -> void;

    [[nodiscard]] auto toBytes() const -> std::vector<std::byte>;

private:
    std::vector<std::byte> version, statusCode, headers, body;
};
