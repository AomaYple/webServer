#pragma once

#include <string>

struct Response {
    Response() noexcept;

    Response(Response &&other) noexcept;

    auto operator=(Response &&other) noexcept -> Response &;

    auto combine() noexcept -> std::string;

    bool parseMethod, parseUrl, parseVersion, writeBody;
    std::string version, statusCode, headers, body;
};
