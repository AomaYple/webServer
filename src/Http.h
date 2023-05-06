#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(const std::string &request) -> std::string;

    Http(const Http &http) = delete;

    Http(Http &&http) = delete;
private:
    Http();

    auto analysisGet(const std::string &page) -> std::string;

    auto analysisPost(const std::string &request) -> std::string;

    static Http http;

    std::unordered_map<std::string, std::string> webpages;
};
