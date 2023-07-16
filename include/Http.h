#pragma once

#include <unordered_map>

#include "Response.h"

class Http {
    static Http instance;

public:
    [[nodiscard]] static auto parse(std::string &&request) -> std::string;

    Http(const Http &) = delete;

    Http(Http &&) = delete;

private:
    static auto parseMethod(Response &response, std::string_view word) -> void;

    static auto parseUrl(Response &response, std::string_view word) -> void;

    static auto parseVersion(Response &response, std::string_view word) -> void;

    Http();

    std::unordered_map<std::string, std::string> webpages;
};
