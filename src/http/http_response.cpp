#include "http_response.hpp"

auto http_response::set_version(std::string_view new_version) -> void {
    this->version.clear();

    this->version.emplace_back(std::byte{'H'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'T'});
    this->version.emplace_back(std::byte{'P'});
    this->version.emplace_back(std::byte{'/'});

    const auto value{std::as_bytes(std::span{new_version})};
    this->version.insert(this->version.cend(), value.cbegin(), value.cend());

    this->version.emplace_back(std::byte{' '});
}

auto http_response::set_status_code(std::string_view new_status_code) -> void {
    this->status_code.clear();

    const auto value{std::as_bytes(std::span{new_status_code})};
    this->status_code.insert(this->status_code.cend(), value.cbegin(), value.cend());

    this->status_code.emplace_back(std::byte{'\r'});
    this->status_code.emplace_back(std::byte{'\n'});
}

auto http_response::add_header(std::string_view header) -> void {
    const auto value{std::as_bytes(std::span{header})};
    this->headers.insert(this->headers.cend(), value.cbegin(), value.cend());

    this->headers.emplace_back(std::byte{'\r'});
    this->headers.emplace_back(std::byte{'\n'});
}

auto http_response::set_body(std::span<const std::byte> new_body) -> void {
    this->body.clear();

    this->body.emplace_back(std::byte{'\r'});
    this->body.emplace_back(std::byte{'\n'});

    this->body.insert(this->body.cend(), new_body.cbegin(), new_body.cend());
}

auto http_response::combine() const -> std::vector<std::byte> {
    std::vector<std::byte> all;

    all.insert(all.cend(), this->version.cbegin(), this->version.cend());
    all.insert(all.cend(), this->status_code.cbegin(), this->status_code.cend());
    all.insert(all.cend(), this->headers.cbegin(), this->headers.cend());
    all.insert(all.cend(), this->body.cbegin(), this->body.cend());

    return all;
}
