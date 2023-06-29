#pragma once

#include <unordered_map>

#include "Response.h"

class Http {
    static Http instance;

public:
    static auto parse(std::string &&request) -> std::string;

    Http(const Http &other) = delete;

    Http(Http &&other) = delete;

private:
    static auto parseMethod(Response &response, std::string_view word) -> bool;

    static auto parseUrl(Response &response, std::string_view word) -> bool;

    static auto parseVersion(Response &response, std::string_view word) -> bool;

    Http();

    std::unordered_map<std::string, std::string> webpages;
};
