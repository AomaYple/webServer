#pragma once

#include <string>
#include <unordered_map>

class Http {
public:
    static auto analysis(std::string_view request) -> std::pair<std::string, bool>;

    Http(const Http &other) = delete;

    Http(Http &&other) = delete;

private:
    struct response {
        std::string line, head, content;
        bool keepAive;
    };

    auto analysisLine(std::string_view request, response &response) -> void;

    Http();

    static Http http;

    std::unordered_map<std::string, std::string> webpages;
};
