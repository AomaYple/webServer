#include "HttpResponse.hpp"

#include <execution>

auto HttpResponse::setVersion(std::string_view newVersion) -> void {
    this->version.resize(newVersion.size());
    std::ranges::transform(newVersion, this->version.begin(),
                           [](const char element) noexcept { return static_cast<std::byte>(element); });

    this->version.emplace_back(std::byte{' '});
}

auto HttpResponse::setStatusCode(std::string_view newStatusCode) -> void {
    this->statusCode.resize(newStatusCode.size());
    std::ranges::transform(newStatusCode, this->statusCode.begin(),
                           [](const char element) noexcept { return static_cast<std::byte>(element); });

    this->statusCode.emplace_back(std::byte{'\r'});
    this->statusCode.emplace_back(std::byte{'\n'});
}

auto HttpResponse::addHeader(std::string_view header) -> void {
    std::ranges::transform(header, std::back_inserter(this->headers),
                           [](const char element) noexcept { return static_cast<std::byte>(element); });

    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto HttpResponse::clearHeaders() noexcept -> void { this->headers.clear(); }

auto HttpResponse::setBody(std::span<const std::byte> newBody) -> void {
    this->body = {std::byte{'\r'}, std::byte{'\n'}};

    this->body.resize(this->body.size() + newBody.size());
    std::copy(std::execution::par_unseq, newBody.cbegin(), newBody.cend(),
              this->body.end() - static_cast<long>(newBody.size()));
}

auto HttpResponse::toBytes() const -> std::vector<std::byte> {
    std::vector<std::byte> bytes;

    std::ranges::copy(this->version, std::back_inserter(bytes));
    std::ranges::copy(this->statusCode, std::back_inserter(bytes));
    std::ranges::copy(this->headers, std::back_inserter(bytes));

    bytes.resize(bytes.size() + this->body.size());
    std::copy(std::execution::par_unseq, this->body.cbegin(), this->body.cend(),
              bytes.end() - static_cast<long>(this->body.size()));

    return bytes;
}
