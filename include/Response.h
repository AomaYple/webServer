#pragma once

#include <string>

struct Response {
    Response();

    Response(Response &&other) noexcept;

    auto operator=(Response &&other) noexcept -> Response &;

    auto combine() -> std::string;

    bool parseMethod, parseUrl, parseVersion, writeBody;
    std::string version, statusCode, headers, body;
};
