#include "HttpResponse.h"

using std::byte, std::string, std::string_view, std::vector, std::span;

auto HttpResponse::setVersion(string_view newVersion) -> void {
    this->version.clear();

    span<const byte> value{as_bytes(span{"HTTP/"})};
    value = value.subspan(0, value.size() - 1);
    this->version.insert(this->version.end(), value.begin(), value.end());

    value = as_bytes(span{newVersion});
    this->version.insert(this->version.end(), value.begin(), value.end());

    value = as_bytes(span{" "});
    value = value.subspan(0, value.size() - 1);
    this->version.insert(this->version.end(), value.begin(), value.end());
}

auto HttpResponse::setStatusCode(string_view newStatusCode) -> void {
    this->statusCode.clear();

    span<const byte> value{as_bytes(span{newStatusCode})};
    this->statusCode.insert(this->statusCode.end(), value.begin(), value.end());

    value = as_bytes(span{"\r\n"});
    value = value.subspan(0, value.size() - 1);
    this->statusCode.insert(this->statusCode.end(), value.begin(), value.end());
}

auto HttpResponse::addHeader(string_view header) -> void {
    span<const byte> value{as_bytes(span{header})};
    this->headers.insert(this->headers.end(), value.begin(), value.end());

    value = as_bytes(span{"\r\n"});
    value = value.subspan(0, value.size() - 1);
    this->headers.insert(this->headers.end(), value.begin(), value.end());
}

auto HttpResponse::setBody(span<const byte> newBody) -> void {
    this->body.clear();

    span<const byte> value{as_bytes(span{"\r\n"})};
    value = value.subspan(0, value.size() - 1);
    this->body.insert(this->body.end(), value.begin(), value.end());

    this->body.insert(this->body.end(), newBody.begin(), newBody.end());
}

auto HttpResponse::combine() -> vector<byte> {
    vector<byte> all;

    all.insert(all.end(), this->version.begin(), this->version.end());
    all.insert(all.end(), this->statusCode.begin(), this->statusCode.end());
    all.insert(all.end(), this->headers.begin(), this->headers.end());
    all.insert(all.end(), this->body.begin(), this->body.end());

    return all;
}
