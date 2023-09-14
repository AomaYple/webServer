#include "HttpResponse.hpp"

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    this->version.clear();

    this->version.emplace_back(std::byte{'H'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'P'});
    this->version.emplace_back(std::byte{'/'});

    const auto value{std::as_bytes(std::span{newVersion})};
    this->version.insert(this->version.end(), value.begin(), value.end());

    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    this->statusCode.clear();

    const auto value{std::as_bytes(std::span{newStatusCode})};
    this->statusCode.insert(this->statusCode.end(), value.begin(), value.end());

    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    const auto value{std::as_bytes(std::span{header})};
    this->headers.insert(this->headers.end(), value.begin(), value.end());

    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body.clear();

    this->body.emplace_back(std::byte{'\r'});
    this->body.emplace_back(std::byte{'\n'});

    this->body.insert(this->body.end(), newBody.begin(), newBody.end());
}

auto HttpResponse::combine() const -> std::vector<std::byte> {
    std::vector<std::byte> all;

    all.insert(all.end(), this->version.begin(), this->version.end());
    all.insert(all.end(), this->statusCode.begin(), this->statusCode.end());
    all.insert(all.end(), this->headers.begin(), this->headers.end());
    all.insert(all.end(), this->body.begin(), this->body.end());

    return all;
}
