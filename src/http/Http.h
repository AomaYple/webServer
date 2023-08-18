#pragma once

#include <source_location>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class Database;
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
    [[nodiscard]] static auto parse(std::span<const std::byte> request, Database &database) -> std::vector<std::byte>;

private:
    static auto parseVersion(HttpResponse &httpResponse, std::string_view version,
                             std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto parseMethod(HttpResponse &httpResponse, std::string_view method,
                            std::source_location sourceLocation = std::source_location::current()) -> void;

    [[nodiscard]] auto parseUrl(HttpResponse &httpResponse, std::string_view url,
                                std::source_location sourceLocation = std::source_location::current()) const
            -> std::span<const std::byte>;

    static auto parseTypeEncoding(HttpResponse &httpResponse, std::string_view url) -> void;

    static auto parseResource(HttpResponse &httpResponse, std::string_view range, std::span<const std::byte> body,
                              bool writeBody, std::source_location sourceLocation = std::source_location::current())
            -> void;

    static const Http instance;

    std::unordered_map<std::string, std::vector<std::byte>> resources;
};
