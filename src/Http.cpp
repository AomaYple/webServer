#include "Http.h"

#include <fstream>
#include <filesystem>
#include <regex>

using std::string, std::to_string, std::pair, std::ifstream, std::stringstream, std::filesystem::directory_iterator,
    std::filesystem::current_path, std::regex, std::smatch, std::regex_search;

Http Http::http;

auto Http::analysis(const string &request) -> pair<string, bool> {
    pair<string, bool> response;
    smatch result;

    string protocol, statusCode, connection, content;

    if (regex_search(request, result, regex("HTTP/(.+)"))) {
        if (result.str(1) == "1.1") {
            protocol = "HTTP/1.1 ";
            connection = "Connection: keep-alive\n";
            response.second = true;
        } else if (result.str(1) == "1.0") {
            protocol = "HTTP/1.0 ";
            connection = "Connection: close\n";
            response.second = false;
        } else
            return {"HTTP/1.0 505 HTTP Version Not Supported\n"
                    "Content-Length: 0\n\n", false};
    } else
        return {"HTTP/1.0 500 Internal Server Error\n"
                "Content-Length: 0\n\n", false};

    if (regex_search(request, result, regex("Connection: (.+)"))) {
        if (result.str(1) == "keep-alive") {
            connection = "Connection: keep-alive\n";
            response.second = true;
        } else if (result.str(1) == "close") {
            connection = "Connection: close\n";
            response.second = false;
        }
    }

    if (regex_search(request, result, regex("GET /(.*) "))) {
        auto findResult {http.webpages.find(result.str(1))};
        if (findResult != http.webpages.end()) {
            statusCode = "200 OK\n";
            content = findResult->second;
        }
        else
            return {"HTTP/1.0 404 Not Found\n"
                    "Content-Length: 0\n\n", false};
    } else
        return {"HTTP/1.0 500 Internal Server Error\n"
                "Content-Length: 0\n\n", false};

    response.first = protocol + statusCode + connection + content;

    return response;
}

Http::Http() {
    this->webpages.emplace("", "Content-Length: 0\n\n");

    for (auto &filePath : directory_iterator(current_path().string() + "/../web")) {
        ifstream file {filePath.path().string()};

        stringstream stream;

        stream << file.rdbuf();

        this->webpages.emplace(filePath.path().string().substr(4), "Content-Length: " + to_string(stream.str().size()) + "\n\n" + stream.str());

        file.close();
    }
}
