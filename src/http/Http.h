#pragma once

#include <source_location>
#include <string>
#include <unordered_map>

class Http {
    static Http instance;

public:
    [[nodiscard]] static auto parse(std::string_view requestString) -> std::string;

    Http(const Http &) = delete;

private:
    static auto gzip(std::string_view data, std::source_location sourceLocation = std::source_location::current())
            -> std::string;

    static auto brotli(std::string_view data, std::source_location sourceLocation = std::source_location::current())
            -> std::string;

    Http();

    std::unordered_map<std::string, std::string> webs;
};
