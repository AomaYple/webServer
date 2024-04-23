#include "HttpResponse.hpp"

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    const std::span<const std::byte> spanNewVersion{std::as_bytes(std::span{newVersion})};

    this->version = {spanNewVersion.cbegin(), spanNewVersion.cend()};
    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    const std::span<const std::byte> spanNewStatusCode{std::as_bytes(std::span{newStatusCode})};

    this->statusCode = {spanNewStatusCode.cbegin(), spanNewStatusCode.cend()};
    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    const std::span<const std::byte> spanHeader{std::as_bytes(std::span{header})};

    this->headers.insert(this->headers.cend(), spanHeader.cbegin(), spanHeader.cend());
    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::clearHeaders() noexcept -> void { this->headers.clear(); }

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body = {std::byte{'\r'}, std::byte{'\n'}};
    this->body.insert(this->body.cend(), newBody.cbegin(), newBody.cend());
}

auto HttpResponse::toByte() const -> std::vector<std::byte> {
    std::vector<std::byte> response;

    response.insert(response.cend(), this->version.cbegin(), this->version.cend());
    response.insert(response.cend(), this->statusCode.cbegin(), this->statusCode.cend());
    response.insert(response.cend(), this->headers.cbegin(), this->headers.cend());
    response.insert(response.cend(), this->body.cbegin(), this->body.cend());

    return response;
}
