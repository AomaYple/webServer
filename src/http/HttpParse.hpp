#pragma once

#include "../database/Database.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class HttpParse {
public:
    explicit HttpParse(Database &&database) noexcept;

    HttpParse(const HttpParse &) = delete;

    auto operator=(const HttpParse &) -> HttpParse & = delete;

    HttpParse(HttpParse &&) noexcept = default;

    auto operator=(HttpParse &&) noexcept -> HttpParse & = default;

    ~HttpParse() = default;

    [[nodiscard]] auto parse(std::string_view request) -> std::vector<std::byte>;

private:
    auto clear() noexcept -> void;

    auto parseVersion() -> void;

    auto parseMethod() -> void;

    auto parsePath() -> void;

    auto parseType(const std::string &resourcePath) -> void;

    auto parseResource(const std::string &resourcePath) -> void;

    auto readResource(const std::string &resourcePath, std::pair<long, long> range,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto brotli(std::source_location sourceLocation = std::source_location::current()) -> void;

    HttpRequest httpRequest{""};
    HttpResponse httpResponse;
    Database database;
    std::vector<std::byte> body;
    bool isWriteBody{true}, isBrotli{false};
};
