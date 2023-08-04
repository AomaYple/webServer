#pragma once

#include <string>

struct Response {
    Response() noexcept;

    [[nodiscard]] auto combine() -> std::string;

    std::string version, statusCode, headers, body;
};
