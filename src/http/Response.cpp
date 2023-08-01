#include "Response.h"

using std::string;

Response::Response() noexcept
    : isParseMethod{false}, isParseUrl{false}, isParseVersion{false}, isWriteBody{false},
      headers{"Connection: keep-alive\r\n"}, body{"\r\n"} {}

auto Response::combine() -> string {
    return std::move(this->version) + std::move(this->statusCode) + std::move(this->headers) + std::move(this->body);
}
