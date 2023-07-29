#pragma once

#include <string>

struct Response {
    Response()

            noexcept;

    [[nodiscard]] auto combine() -> std::string;

    bool isParseMethod, isParseUrl, isParseVersion, isWriteBody;
    std::string version, statusCode, headers, body;
};
