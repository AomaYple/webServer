#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(const std::string &request) -> std::pair<std::string, bool>;

    Http(const Http &http) = delete;

    Http(Http &&http) = delete;

private:
    Http();

    static Http http;

    std::unordered_map<std::string, std::string> webpages;
};
