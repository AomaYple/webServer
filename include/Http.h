#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(std::string_view request) -> std::string;

    Http(const Http &other) = delete;

    Http(Http &&other) = delete;

private:
    Http();

    static Http http;

    std::unordered_map<std::string, std::string> webpages;
};
