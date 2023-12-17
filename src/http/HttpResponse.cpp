#include "HttpResponse.hpp"

static constexpr std::array<const std::byte, 2> delimiter{std::byte{'\r'}, std::byte{'\n'}};

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    const std::span<const std::byte> value{std::as_bytes(std::span{newVersion})};
    this->version = {value.cbegin(), value.cend()};

    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    const std::span<const std::byte> value{std::as_bytes(std::span{newStatusCode})};
    this->statusCode = {value.cbegin(), value.cend()};

    this->statusCode.insert(this->statusCode.cend(), delimiter.cbegin(), delimiter.cend());
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    const std::span<const std::byte> value{std::as_bytes(std::span{header})};
    this->headers.insert(this->headers.cend(), value.cbegin(), value.cend());

    this->headers.insert(this->headers.cend(), delimiter.cbegin(), delimiter.cend());
}

auto HttpResponse::clearHeaders() noexcept -> void { this->headers.clear(); }

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body = {delimiter.cbegin(), delimiter.cend()};
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
