#pragma once

#include <span>
#include <string_view>
#include <vector>

class http_response {
public:
    constexpr http_response() noexcept = default;

    auto set_version(std::string_view new_version) -> void;

    auto set_status_code(std::string_view new_status_code) -> void;

    auto add_header(std::string_view header) -> void;

    auto set_body(std::span<const std::byte> new_body) -> void;

    [[nodiscard]] auto combine() const -> std::vector<std::byte>;

private:
    std::vector<std::byte> version, status_code, headers, body;
};
