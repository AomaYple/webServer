#include "Response.h"

using std::string;

Response::Response()
    : parseMethod{false}, parseUrl{false}, parseVersion{false}, writeBody{false},
      headers{"Content-Type: text/html; charset=utf-8\r\n"}, body{"\r\n"} {}

Response::Response(Response &&other) noexcept
    : parseMethod{other.parseMethod}, parseUrl{other.parseUrl}, parseVersion{other.parseVersion},
      writeBody{other.writeBody}, version{std::move(other.version)}, statusCode{std::move(other.statusCode)},
      headers{std::move(other.headers)}, body{std::move(other.body)} {}

auto Response::operator=(Response &&other) noexcept -> Response & {
    if (this != &other) {
        this->parseMethod = other.parseMethod;
        this->parseUrl = other.parseUrl;
        this->parseVersion = other.parseVersion;
        this->writeBody = other.writeBody;
        this->version = std::move(other.version);
        this->statusCode = std::move(other.statusCode);
        this->headers = std::move(other.headers);
        this->body = std::move(other.body);
    }
    return *this;
}

auto Response::combine() -> string {
    return std::move(version) + std::move(statusCode) + std::move(headers) + std::move(body);
}
