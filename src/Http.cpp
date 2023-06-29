#include "Http.h"

#include <filesystem>
#include <fstream>
#include <ranges>

using std::string, std::string_view, std::to_string, std::array, std::ifstream, std::ostringstream,
        std::filesystem::directory_iterator, std::filesystem::current_path, std::views::split, std::ranges::find_if;

Http Http::instance;

auto Http::parse(string &&request) -> string {
    Response response;

    constexpr string_view delimiter{"\r\n"};

    for (const auto &lineView: request | split(delimiter))
        for (const auto &wordView: lineView | split(' ')) {
            string_view word{wordView};

            if (!response.parseMethod) {
                if (!Http::parseMethod(response, word)) return response.combine();

            } else if (!response.parseUrl) {
                if (!Http::parseUrl(response, word)) return response.combine();
            } else if (!response.parseVersion) {
                if (!Http::parseVersion(response, word)) return response.combine();
            } else
                return response.combine();
        }


    return response.combine();
}

auto Http::parseMethod(Response &response, string_view word) -> bool {
    constexpr array<string_view, 2> methods{"GET", "HEAD"};

    auto result{find_if(methods, [word](string_view method) { return method == word; })};

    if (result != methods.end()) {
        response.writeBody = *result == "GET";

        response.parseMethod = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "501 Not Implemented\r\n";
        response.headers += "Content-Length: 0\r\n";
    }

    return response.parseMethod;
}

auto Http::parseUrl(Response &response, string_view word) -> bool {
    string_view pageName{word.substr(1)};

    auto page{Http::instance.webpages.find(string{pageName})};

    if (page != Http::instance.webpages.end()) {
        response.statusCode = "200 OK\r\n";
        response.headers += "Content-Length: " + to_string(page->second.size()) + "\r\n";
        response.body += response.writeBody ? page->second : "";

        response.parseUrl = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "404 Not Found\r\n";
        response.headers += "Content-Length: 0\r\n";
    }

    return response.parseUrl;
}

auto Http::parseVersion(Response &response, string_view word) -> bool {
    constexpr array<string_view, 1> versions{"HTTP/1.1"};

    auto result{find_if(versions, [word](string_view version) { return version == word; })};

    if (result != versions.end()) {
        response.version = string{*result} + " ";

        response.parseVersion = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "505 HTTP Version Not Supported\r\n";
        response.headers += "Content-Length: 0\r\n";
    }

    return response.parseVersion;
}

Http::Http() {
    string webPagesPath{current_path().string() + "/../web/"};

    for (const auto &webPagePath: directory_iterator(webPagesPath)) {
        ifstream file{webPagePath.path().string()};

        ostringstream stream;

        stream << file.rdbuf();

        this->webpages.emplace(webPagePath.path().string().substr(webPagesPath.size()), stream.str());
    }

    this->webpages.emplace("", "");
}
