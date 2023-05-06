#include "Http.h"

#include <fstream>
#include <regex>
#include <filesystem>

using std::string, std::to_string, std::pair, std::ifstream, std::ostringstream, std::regex, std::smatch, std::filesystem::directory_iterator,
    std::filesystem::current_path;

auto Http::analysis(const std::string &request) -> std::string {
    smatch result;

    if (regex_search(request, result, regex("(.+) /(.*) HTTP/(.+)\r"))) {
        if (result.str(3) == "1.1") {
            if (result.str(1) == "GET")
                return http.analysisGet(result.str(2));
            else if (result.str(1) == "POST")
                return http.analysisPost(request);
            else
                return {"HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n"};
        } else
            return {"HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n"};
    } else
        return {"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"};
}

Http::Http() {
    string path {current_path().string() + "/../web/"};

    for (auto &filePath : directory_iterator(path)) {
        ifstream file {filePath.path().string()};

        ostringstream stream;

        stream << file.rdbuf();

        this->webpages.emplace(filePath.path().string().substr(path.size()), "Content-Length: " + to_string(stream.str().size()) + "\r\n\r\n" + stream.str());
    }

    this->webpages.emplace("", this->webpages["index.html"]);
}

auto Http::analysisGet(const string &page) -> string {
    auto findResult {this->webpages.find(page)};

    if (findResult != this->webpages.end())
        return {"HTTP/1.1 200 OK\r\n" + findResult->second};
    else
        return {"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"};
}

auto Http::analysisPost(const string &request) -> string {
    return {"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"};
}

Http Http::http;
