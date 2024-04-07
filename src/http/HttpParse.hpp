#pragma once

#include "Database.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class HttpParse {
public:
    HttpParse();

    HttpParse(const HttpParse &) = delete;

    auto operator=(const HttpParse &) -> HttpParse & = delete;

    HttpParse(HttpParse &&) = default;

    auto operator=(HttpParse &&) -> HttpParse & = default;

    ~HttpParse() = default;

    [[nodiscard]] auto parse(std::string_view request,
                             std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

private:
    auto clear() noexcept -> void;

    auto parseVersion() -> void;

    auto parseMethod() -> void;

    auto parsePath() -> void;

    auto parseResource(const std::string &resourcePath) -> void;

    auto readResource(const std::string &resourcePath, std::pair<long, long> range,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto brotli(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleException() -> void;

    HttpRequest httpRequest;
    HttpResponse httpResponse;
    Database database;
    std::vector<std::byte> body;
    bool wroteBody{true}, isBrotli{};
};
