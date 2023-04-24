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

    if (regex_search(request, result, regex("GET /(.*) HTTP/(.+)\r"))) {
        auto findResult {http.webpages.find(result.str(1))};

        if (findResult != http.webpages.end()) {
            statusCode = "200 OK\r\n";
            content = findResult->second;
        }
        else {
            statusCode = "404 Not Found\r\n";
            content = "Content-Length: 0\r\n\r\n";
        }

        if (result[2].str() == "1.0") {
            protocol = "HTTP/1.0 ";
            response.second = false;
        } else if (result[2].str() == "1.1") {
            protocol = "HTTP/1.1 ";
            response.second = true;
        }

    } else
        return {"HTTP/1.0 500 Internal Server Error\n"
                "Content-Length: 0\n\n", false};

    if (regex_search(request, result, regex("Connection: (.+)\r"))) {
        if (result.str(1) == "keep-alive") {
            connection = "Connection: keep-alive\r\n";
            response.second = true;
        } else if (result.str(1) == "close") {
            connection = "Connection: close\r\n";
            response.second = false;
        }
    }

    response.first = protocol + statusCode + connection + content;

    return response;
}

Http::Http() {
    this->webpages.emplace("", "Content-Length: 0\r\n\r\n");

    string path {current_path().string() + "/../web/"};

    for (auto &filePath : directory_iterator(path)) {
        ifstream file {filePath.path().string()};

        stringstream stream;

        stream << file.rdbuf();

        this->webpages.emplace(filePath.path().string().substr(path.size()), "Content-Length: " + to_string(stream.str().size())
            + "\r\n\r\n" + stream.str());
    }
}
