#pragma once

#include <cstdint>
#include <queue>
#include <source_location>
#include <span>
#include <string>
#include <unordered_map>

class Database;
class HttpRequest;
class HttpResponse;

class Http {
    Http();

    [[nodiscard]] static auto readFile(std::string_view filepath,
                                       std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

    [[nodiscard]] static auto brotli(std::span<const std::byte> data,
                                     std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

public:
    [[nodiscard]] static auto parse(std::span<const std::byte> request, Database &database)
            -> std::queue<std::vector<std::byte>>;

private:
    [[nodiscard]] static auto parseVersion(HttpResponse &httpResponse, const HttpRequest &httpRequest,
                                           std::source_location sourceLocation = std::source_location::current())
            -> std::queue<std::vector<std::byte>>;

    [[nodiscard]] static auto parseMethod(HttpResponse &httpResponse, const HttpRequest &httpRequest,
                                          std::source_location sourceLocation = std::source_location::current())
            -> std::queue<std::vector<std::byte>>;

    [[nodiscard]] auto parseUrl(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody,
                                std::source_location sourceLocation = std::source_location::current()) const
            -> std::queue<std::vector<std::byte>>;

    static auto parseTypeEncoding(std::string_view url, HttpResponse &httpResponse) -> void;

    [[nodiscard]] static auto parseResource(HttpResponse &httpResponse, const HttpRequest &httpRequest,
                                            std::span<const std::byte> body, bool writeBody,
                                            std::source_location sourceLocation = std::source_location::current())
            -> std::queue<std::vector<std::byte>>;

    static auto parseRange(HttpResponse &httpResponse, std::uint_least32_t maxSize, std::string_view range,
                           std::span<const std::byte> body, bool writeBody,
                           std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] static auto parseOversize(HttpResponse &httpResponse, std::uint_least32_t maxSize,
                                            std::span<const std::byte> body, bool writeBody)
            -> std::queue<std::vector<std::byte>>;

    [[nodiscard]] static auto decimalToHexadecimal(std::int_fast64_t number) -> std::string;

    static auto parseNormal(HttpResponse &httpResponse, std::span<const std::byte> body, bool writeBody) -> void;

    static const Http instance;

    std::unordered_map<std::string, std::vector<std::byte>> resources;
};
