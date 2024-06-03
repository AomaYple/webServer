#include "HttpResponse.hpp"

auto HttpResponse::setVersion(const std::string_view version) -> void {
    const auto spanNewVersion{std::as_bytes(std::span{version})};

    this->version = {spanNewVersion.cbegin(), spanNewVersion.cend()};
    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(const std::string_view statusCode) -> void {
    const auto spanNewStatusCode{std::as_bytes(std::span{statusCode})};

    this->statusCode = {spanNewStatusCode.cbegin(), spanNewStatusCode.cend()};
    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(const std::string_view header) -> void {
    const auto spanHeader{std::as_bytes(std::span{header})};

    this->headers.insert(this->headers.cend(), spanHeader.cbegin(), spanHeader.cend());
    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::clearHeaders() noexcept -> void { this->headers.clear(); }

auto HttpResponse::setBody(const std::span<const std::byte> body) -> void {
    this->body = {std::byte{'\r'}, std::byte{'\n'}};
    this->body.insert(this->body.cend(), body.cbegin(), body.cend());
}

auto HttpResponse::toByte() const -> std::vector<std::byte> {
    std::vector<std::byte> response;

    response.insert(response.cend(), this->version.cbegin(), this->version.cend());
    response.insert(response.cend(), this->statusCode.cbegin(), this->statusCode.cend());
    response.insert(response.cend(), this->headers.cbegin(), this->headers.cend());
    response.insert(response.cend(), this->body.cbegin(), this->body.cend());

    return response;
}
