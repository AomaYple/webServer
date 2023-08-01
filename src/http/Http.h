#pragma once

#include <source_location>
#include <string>
#include <unordered_map>

class Response;

class Http {
    static Http instance;

public:
    [[nodiscard]] static auto parse(std::string_view request) -> std::string;

    Http(const Http &) = delete;

private:
    static auto parseMethod(Response &response, std::string_view word,
                            std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto parseUrl(Response &response, std::string_view word,
                         std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto parseVersion(Response &response, std::string_view word,
                             std::source_location sourceLocation = std::source_location::current()) -> void;

    Http();

    static auto gzipCompress(std::string_view content,
                             std::source_location sourceLocation = std::source_location::current()) -> std::string;

    std::unordered_map<std::string, std::string> webs;
};
