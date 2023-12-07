#pragma once

#include <span>
#include <string_view>
#include <vector>

class HttpResponse {
public:
    constexpr HttpResponse() noexcept = default;

    auto setVersion(std::string_view newVersion) noexcept -> void;

    auto setStatusCode(std::string_view newStatusCode) noexcept -> void;

    auto addHeader(std::string_view header) noexcept -> void;

    auto setBody(std::span<const std::byte> newBody) noexcept -> void;

    [[nodiscard]] auto toBytes() const noexcept -> std::vector<std::byte>;

private:
    std::vector<std::byte> version, statusCode, headers, body;
};
