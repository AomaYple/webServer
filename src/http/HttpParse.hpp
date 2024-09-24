#pragma once

#include "Database.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Logger;

class HttpParse {
public:
    explicit HttpParse(const std::shared_ptr<Logger> &logger);

    HttpParse(const HttpParse &) = delete;

    HttpParse(HttpParse &&) noexcept = default;

    auto operator=(const HttpParse &) -> HttpParse & = delete;

    auto operator=(HttpParse &&) noexcept -> HttpParse & = default;

    ~HttpParse() = default;

    [[nodiscard]] auto parse(std::string_view request,
                             std::source_location sourceLocation = std::source_location::current())
        -> std::vector<std::byte>;

private:
    auto clear() -> void;

    auto parseVersion() -> void;

    auto parseMethod() -> void;

    auto parsePath() -> void;

    auto parseResource(const std::string &resourcePath) -> void;

    auto readResource(const std::string &resourcePath, const std::pair<long, long> &range,
                      std::source_location sourceLocation = std::source_location::current()) -> void;

    auto brotli(std::source_location sourceLocation = std::source_location::current()) -> void;

    auto handleException() -> void;

    HttpRequest httpRequest;
    HttpResponse httpResponse;
    Database database;
    std::vector<std::byte> body;
    bool isWroteBody{true}, isBrotli{};
    std::shared_ptr<Logger> logger;
};
