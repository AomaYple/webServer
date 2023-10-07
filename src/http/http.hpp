#pragma once

#include <source_location>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class Database;
class http_response;
class http_request;

class http {
    http();

    [[nodiscard]] static auto readFile(std::string_view filepath,
                                       std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

    [[nodiscard]] static auto brotli(std::span<const std::byte> data,
                                     std::source_location sourceLocation = std::source_location::current())
            -> std::vector<std::byte>;

public:
    [[nodiscard]] static auto parse(std::string_view request, Database &database) -> std::vector<std::byte>;

private:
    static auto parseVersion(http_response &httpResponse, std::string_view version,
                             std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto parseGetHead(http_response &httpResponse, const http_request &httpRequest, bool writeBody) -> void;

    [[nodiscard]] auto parseUrl(http_response &httpResponse, std::string_view url,
                                std::source_location sourceLocation = std::source_location::current()) const
            -> std::span<const std::byte>;

    static auto parseTypeEncoding(http_response &httpResponse, std::string_view url) -> void;

    static auto parseResource(http_response &httpResponse, std::string_view range, std::span<const std::byte> body,
                              bool writeBody, std::source_location sourceLocation = std::source_location::current())
            -> void;

    static auto parsePost(http_response &httpResponse, std::string_view message, Database &database) -> void;

    static auto parseLogin(http_response &httpResponse, std::string_view id, std::string_view password,
                           Database &database) -> void;

    static auto parseRegister(http_response &httpResponse, std::string_view password, Database &database) -> void;

    static const http instance;

    std::unordered_map<std::string, std::vector<std::byte>> resources;
};
