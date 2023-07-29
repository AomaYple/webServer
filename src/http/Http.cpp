#include "Http.h"

#include <filesystem>
#include <fstream>
#include <ranges>

using std::array, std::string, std::string_view, std::to_string;
using std::ifstream, std::ostringstream, std::filesystem::directory_iterator, std::filesystem::current_path;
using std::invalid_argument;
using std::views::split, std::ranges::find_if;

Http Http::instance;

auto Http::parse(string &&request) -> string {
    Response response;

    constexpr string_view delimiter{"\r\n"};

    for (const auto &lineView: request | split(delimiter))
        for (const auto &wordView: lineView | split(' ')) {
            string_view word{wordView};

            if (!response.isParseMethod) {
                try {
                    Http::parseMethod(response, word);
                } catch (const invalid_argument &invalidArgument) { return response.combine(); }
            } else if (!response.isParseUrl) {
                try {
                    Http::parseUrl(response, word);
                } catch (const invalid_argument &invalidArgument) { return response.combine(); }
            } else if (!response.isParseVersion) {
                try {
                    Http::parseVersion(response, word);
                } catch (const invalid_argument &invalidArgument) { return response.combine(); }
            } else
                return response.combine();
        }

    return response.combine();
}

auto Http::parseMethod(Response &response, string_view word) -> void {
    constexpr array<string_view, 2> methods{"GET", "HEAD"};

    auto result{find_if(methods, [word](string_view method) { return method == word; })};

    if (result != methods.end()) {
        response.isWriteBody = *result == "GET";

        response.isParseMethod = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "501 Not Implemented\r\n";
        response.headers += "Content-Length: 0\r\n";

        throw invalid_argument("invalid method");
    }
}

auto Http::parseUrl(Response &response, string_view word) -> void {
    string_view pageName{word.substr(1)};

    auto page{Http::instance.webpages.find(string{pageName})};

    if (page != Http::instance.webpages.end()) {
        response.statusCode = "200 OK\r\n";
        response.headers += "Content-Length: " + to_string(page->second.size()) + "\r\n";
        response.body += response.isWriteBody ? page->second : "";

        response.isParseUrl = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "404 Not Found\r\n";
        response.headers += "Content-Length: 0\r\n";

        throw invalid_argument("invalid url");
    }
}

auto Http::parseVersion(Response &response, string_view word) -> void {
    constexpr array<string_view, 1> versions{"HTTP/1.1"};

    auto result{find_if(versions, [word](string_view version) { return version == word; })};

    if (result != versions.end()) {
        response.version = string{*result} + " ";

        response.isParseVersion = true;
    } else {
        response.version = "HTTP/1.1 ";
        response.statusCode = "505 HTTP Version Not Supported\r\n";
        response.headers += "Content-Length: 0\r\n";

        throw invalid_argument("invalid version");
    }
}

Http::Http() {
    for (const auto &page: directory_iterator(current_path().string() + "/web")) {
        ifstream file{page.path()};

        ostringstream stream;

        stream << file.rdbuf();
        this->webpages.emplace(page.path().filename(), stream.str());
    }

    this->webpages.emplace("", "");
}
