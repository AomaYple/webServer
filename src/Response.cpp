#include "Response.h"

using std::string;

Response::Response()
    : isParseMethod{false}, isParseUrl{false}, isParseVersion{false}, isWriteBody{false},
      headers{"Content-Type: text/html; charset=utf-8\r\n"}, body{"\r\n"} {}

Response::Response(Response &&other) noexcept
    : isParseMethod{other.isParseMethod}, isParseUrl{other.isParseUrl}, isParseVersion{other.isParseVersion},
      isWriteBody{other.isWriteBody}, version{std::move(other.version)}, statusCode{std::move(other.statusCode)},
      headers{std::move(other.headers)}, body{std::move(other.body)} {}

auto Response::operator=(Response &&other) noexcept -> Response & {
    if (this != &other) {
        this->isParseMethod = other.isParseMethod;
        this->isParseUrl = other.isParseUrl;
        this->isParseVersion = other.isParseVersion;
        this->isWriteBody = other.isWriteBody;
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
