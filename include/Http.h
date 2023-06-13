#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(const std::string &request) -> std::pair<std::string, bool>;

    Http(const Http &other) = delete;

    Http(Http &&other) = delete;

private:
    Http();

    static Http http;

    std::unordered_map<std::string, std::string> webpages;
};
