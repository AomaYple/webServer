#pragma once

#include <string>

struct Response {
    Response();

    Response(Response &&) noexcept;

    auto operator=(Response &&) noexcept -> Response &;

    auto combine() -> std::string;

    bool isParseMethod, isParseUrl, isParseVersion, isWriteBody;
    std::string version, statusCode, headers, body;
};
