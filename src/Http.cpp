#include "Http.h"

#include <filesystem>
#include <fstream>

using std::string, std::string_view, std::to_string, std::ifstream, std::ostringstream,
        std::filesystem::directory_iterator, std::filesystem::current_path;

auto Http::analysis(string_view request) -> string {
    return {"HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n\r\n"};
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
