#include "Http.h"

#include <filesystem>
#include <fstream>
#include <regex>

using std::string, std::to_string, std::pair, std::ifstream, std::ostringstream,
    std::regex, std::smatch, std::regex_search,
    std::filesystem::directory_iterator, std::filesystem::current_path;

auto Http::analysis(const string& request) -> pair<string, bool> {
  pair<string, bool> response;

  string head, content;

  smatch result;

  if (regex_search(request, result, regex("(.+) /(.*) HTTP/(.+)\r\n"))) {
    if (result.str(3) == "1.1") {
      response.first += "HTTP/1.1 ";
      response.second = true;
    } else if (result.str(3) == "1.0") {
      response.first += "HTTP/1.0 ";
      response.second = false;
    } else
      return {
          "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: "
          "0\r\n\r\n",
          true};

    if (result.str(1) == "GET") {
      auto findResult{http.webpages.find(result.str(2))};

      if (findResult != http.webpages.end()) {
        response.first += "200 OK\r\n";
        content = findResult->second;
      } else
        return {"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", true};
    } else
      return {"HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n",
              true};
  } else
    return {"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", true};

  if (regex_search(request, result, regex("Connection: (.+)\r\n"))) {
    if (result.str(1) == "keep-alive") {
      head += "Connection: keep-alive\r\n";
      response.second = true;
    } else if (result.str(1) == "close") {
      head += "Connection: close\r\n";
      response.second = false;
    } else
      return {"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n", true};
  }

  response.first += head + content;

  return response;
}

Http::Http() {
  string path{current_path().string() + "/../web/"};

  for (auto& filePath : directory_iterator(path)) {
    ifstream file{filePath.path().string()};

    ostringstream stream;

    stream << file.rdbuf();

    this->webpages.emplace(filePath.path().string().substr(path.size()),
                           "Content-Length: " + to_string(stream.str().size()) +
                               "\r\n\r\n" + stream.str());
  }

  this->webpages.emplace("", "Content-Length: 0\r\n\r\n");
}

Http Http::http;
