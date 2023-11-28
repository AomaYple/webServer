#include "HttpResponse.hpp"

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    this->version.clear();

    this->version.emplace_back(std::byte{'H'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'P'});
    this->version.emplace_back(std::byte{'/'});

    const std::span<const std::byte> value{std::as_bytes(std::span{newVersion})};
    this->version.insert(this->version.cend(), value.cbegin(), value.cend());

    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    this->statusCode.clear();

    const std::span<const std::byte> value{std::as_bytes(std::span{newStatusCode})};
    this->statusCode.insert(this->statusCode.cend(), value.cbegin(), value.cend());

    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    const std::span<const std::byte> value{std::as_bytes(std::span{header})};
    this->headers.insert(this->headers.cend(), value.cbegin(), value.cend());

    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body.clear();

    this->body.emplace_back(std::byte{'\r'});
    this->body.emplace_back(std::byte{'\n'});

    this->body.insert(this->body.cend(), newBody.cbegin(), newBody.cend());
}

auto HttpResponse::toBytes() const -> std::vector<std::byte> {
    std::vector<std::byte> all;

    all.insert(all.cend(), this->version.cbegin(), this->version.cend());
    all.insert(all.cend(), this->statusCode.cbegin(), this->statusCode.cend());
    all.insert(all.cend(), this->headers.cbegin(), this->headers.cend());
    all.insert(all.cend(), this->body.cbegin(), this->body.cend());

    return all;
}
