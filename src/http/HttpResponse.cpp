#include "HttpResponse.h"

using namespace std;

auto HttpResponse::setVersion(string_view newVersion) noexcept -> void {
    this->version.clear();

    this->version.emplace_back(byte{'H'});
    this->version.emplace_back(byte{'T'});
    this->version.emplace_back(byte{'T'});
    this->version.emplace_back(byte{'P'});
    this->version.emplace_back(byte{'/'});

    const auto value{as_bytes(span{newVersion})};
    this->version.insert(this->version.end(), value.begin(), value.end());

    this->version.emplace_back(byte{' '});
}

auto HttpResponse::setStatusCode(string_view newStatusCode) noexcept -> void {
    this->statusCode.clear();

    const auto value{as_bytes(span{newStatusCode})};
    this->statusCode.insert(this->statusCode.end(), value.begin(), value.end());

    this->statusCode.emplace_back(byte{'\r'});
    this->statusCode.emplace_back(byte{'\n'});
}

auto HttpResponse::addHeader(string_view header) noexcept -> void {
    const auto value{as_bytes(span{header})};
    this->headers.insert(this->headers.end(), value.begin(), value.end());

    this->headers.emplace_back(byte{'\r'});
    this->headers.emplace_back(byte{'\n'});
}

auto HttpResponse::setBody(span<const byte> newBody) noexcept -> void {
    this->body.clear();

    this->body.emplace_back(byte{'\r'});
    this->body.emplace_back(byte{'\n'});

    this->body.insert(this->body.end(), newBody.begin(), newBody.end());
}

auto HttpResponse::combine() const noexcept -> vector<byte> {
    vector<byte> all;

    all.insert(all.end(), this->version.begin(), this->version.end());
    all.insert(all.end(), this->statusCode.begin(), this->statusCode.end());
    all.insert(all.end(), this->headers.begin(), this->headers.end());
    all.insert(all.end(), this->body.begin(), this->body.end());

    return all;
}
