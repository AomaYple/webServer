#pragma once

#include <source_location>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class Database;
class HttpResponse;
class HttpRequest;

class Http {
    Http();

    static auto readFile(std::string_view filepath,
                         std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

    static auto brotli(std::span<const std::byte> data,
                       std::source_location sourceLocation = std::source_location::current()) -> std::vector<std::byte>;

public:
    static auto parse(std::string_view request, Database &database) -> std::vector<std::byte>;

private:
    static auto parseVersion(HttpResponse &httpResponse, std::string_view version,
                             std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody) -> void;

    auto parseUrl(HttpResponse &httpResponse, std::string_view url,
                  std::source_location sourceLocation = std::source_location::current()) const
            -> std::span<const std::byte>;

    static auto parseTypeEncoding(HttpResponse &httpResponse, std::string_view url) -> void;

    static auto parseResource(HttpResponse &httpResponse, std::string_view range, std::span<const std::byte> body,
                              bool writeBody, std::source_location sourceLocation = std::source_location::current())
            -> void;

    static auto parsePost(HttpResponse &httpResponse, std::string_view message, Database &database) -> void;

    static const Http instance;

    std::unordered_map<std::string, std::vector<std::byte>> resources;
};
