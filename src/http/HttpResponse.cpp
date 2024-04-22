#include "HttpResponse.hpp"

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    const std::span<const std::byte> newVersionSpan{std::as_bytes(std::span{newVersion})};
    this->version = {newVersionSpan.cbegin(), newVersionSpan.cend()};

    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    const std::span<const std::byte> newStatusCodeSpan{std::as_bytes(std::span{newStatusCode})};
    this->statusCode = {newStatusCodeSpan.cbegin(), newStatusCodeSpan.cend()};

    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    const std::span<const std::byte> headerSpan{std::as_bytes(std::span{header})};
    this->headers.insert(this->headers.cend(), headerSpan.cbegin(), headerSpan.cend());

    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::clearHeaders() noexcept -> void { this->headers.clear(); }

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body = {std::byte{'\r'}, std::byte{'\n'}};
    this->body.insert(this->body.cend(), newBody.cbegin(), newBody.cend());
}

auto HttpResponse::toBytes() const -> std::vector<std::byte> {
    std::vector<std::byte> bytes;

    bytes.insert(bytes.cend(), this->version.cbegin(), this->version.cend());
    bytes.insert(bytes.cend(), this->statusCode.cbegin(), this->statusCode.cend());
    bytes.insert(bytes.cend(), this->headers.cbegin(), this->headers.cend());
    bytes.insert(bytes.cend(), this->body.cbegin(), this->body.cend());

    return bytes;
}
