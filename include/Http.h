#ifndef WEBSERVER_HTTP_H
#define WEBSERVER_HTTP_H

#include <string>
#include <unordered_map>

class Http {
 public:
  static auto analysis(const std::string& request)
      -> std::pair<std::string, bool>;

  Http(const Http& http) = delete;

  Http(Http&& http) = delete;

 private:
  Http();

  static Http http;

  std::unordered_map<std::string, std::string> webpages;
};

#endif  //WEBSERVER_HTTP_H
