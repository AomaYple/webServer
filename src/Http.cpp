#include "Http.h"

#include <filesystem>
#include <fstream>

#include "DataBase.h"

using std::string, std::string_view, std::to_string, std::pair, std::ifstream, std::ostringstream,
        std::filesystem::directory_iterator, std::filesystem::current_path;

auto Http::analysis(string_view request) -> pair<string, bool> {
    pair<string, bool> response;
    return response;
}

auto Http::analysisLine(string_view request, response &response) -> void {
    string protocol{"HTTP/"}, statusCode;

    for (unsigned int i{0}; i < request.size() && request[i] != '\r'; ++i) {
        if (i == 0 && request[i] == 'G') {}
    }

    response.line = protocol + statusCode;
}

Http::Http() {
    string path{current_path().string() + "/../web/"};

    for (auto &filePath: directory_iterator(path)) {
        ifstream file{filePath.path().string()};

        ostringstream stream;

        stream << file.rdbuf();

        this->webpages.emplace(filePath.path().string().substr(path.size()),
                               "Content-Length: " + to_string(stream.str().size()) + "\r\n\r\n" + stream.str());
    }

    this->webpages.emplace("", "Content-Length: 0\r\n\r\n");
}

Http Http::http;
