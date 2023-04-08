#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(const std::string &request) -> std::pair<std::string, bool>;
private:
    std::unordered_map<std::string, std::string> webpages;

    static Http http;

    Http();
};
