#include "http.hpp"

#include "../database/Database.hpp"
#include "../log/logger.hpp"
#include "http_parse_error.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <brotli/encode.h>

#include <filesystem>
#include <format>
#include <ranges>

http::http()
    : resources{[] {
          std::unordered_map<std::string, std::vector<std::byte>> tempResources;

          tempResources.emplace("", std::vector<std::byte>{});

          for (const auto &path:
               std::filesystem::directory_iterator(std::filesystem::current_path().string() + "/resources"))
              for (const auto &subPath: std::filesystem::directory_iterator(path)) {
                  auto filename{subPath.path().filename().string()};

                  std::vector<std::byte> fileContent{http::readFile(subPath.path().string())};

                  if (filename.ends_with("html")) fileContent = http::brotli(fileContent);

                  tempResources.emplace(std::move(filename), std::move(fileContent));
              }

          return tempResources;
      }()} {}

auto http::readFile(std::string_view filepath, std::source_location sourceLocation) -> std::vector<std::byte> {
    std::ifstream file{std::string{filepath}, std::ios::binary | std::ios::ate};
    if (!file) throw std::runtime_error{"file not found: " + std::string{filepath}};

    const auto size{static_cast<unsigned long>(file.tellg())};
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size, std::byte{0});

    file.read(reinterpret_cast<char *>(buffer.data()), static_cast<long>(size));
    if (file.gcount() != static_cast<long>(size)) throw std::runtime_error{"file read error: " + std::string{filepath}};

    return buffer;
}

auto http::brotli(std::span<const std::byte> data, std::source_location sourceLocation) -> std::vector<std::byte> {
    unsigned long size{BrotliEncoderMaxCompressedSize(data.size())};

    std::vector<std::byte> buffer(size, std::byte{0});

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const unsigned char *>(data.data()), &size,
                              reinterpret_cast<unsigned char *>(buffer.data())) != BROTLI_TRUE)
        throw std::runtime_error{"brotli compression error"};

    buffer.resize(size);

    return buffer;
}

auto http::parse(std::string_view request, Database &database) -> std::vector<std::byte> {
    http_response httpResponse;

    const http_request httpRequest{request};

    try {
        http::parseVersion(httpResponse, httpRequest.get_version());

        const std::string_view method{httpRequest.get_method()};

        if (method == "GET" || method == "HEAD") http::parseGetHead(httpResponse, httpRequest, method == "GET");
        else if (method == "POST") {
            http::parsePost(httpResponse, httpRequest.get_body(), database);
        } else {
            httpResponse.set_status_code("405 Method Not Allowed");
            httpResponse.add_header("Content-Length: 0");
            httpResponse.set_body({});

            throw http_parse_error{<#initializer #>};
        }
    } catch (const http_parse_error &httpParseError) { logger::produce(<#initializer #>); }

    return httpResponse.combine();
}

auto http::parseVersion(http_response &httpResponse, std::string_view version, std::source_location sourceLocation)
        -> void {
    if (version != "1.1") {
        httpResponse.set_version("1.1");
        httpResponse.set_status_code("505 HTTP Version Not Supported");
        httpResponse.add_header("Content-Length: 0");
        httpResponse.set_body({});

        throw http_parse_error{<#initializer #>};
    }

    httpResponse.set_version(version);
}

auto http::parseGetHead(http_response &httpResponse, const http_request &httpRequest, bool writeBody) -> void {
    const std::string_view url{httpRequest.get_url()};

    http::parseTypeEncoding(httpResponse, url);

    const std::span<const std::byte> body{http::instance.parseUrl(httpResponse, url)};
    http::parseResource(httpResponse, httpRequest.get_header_value("Range"), body, writeBody);
}

auto http::parseUrl(http_response &httpResponse, std::string_view url, std::source_location sourceLocation) const
        -> std::span<const std::byte> {
    const auto result{this->resources.find(std::string{url})};

    if (result == this->resources.cend()) {
        httpResponse.set_status_code("404 Not Found");
        httpResponse.add_header("Content-Length: 0");
        httpResponse.set_body({});

        throw http_parse_error{<#initializer #>};
    }

    return result->second;
}

auto http::parseTypeEncoding(http_response &httpResponse, std::string_view url) -> void {
    if (url.ends_with("html")) {
        httpResponse.add_header("Content-Type: text/html; charset=utf-8");
        httpResponse.add_header("Content-Encoding: br");
    } else if (url.ends_with("jpg"))
        httpResponse.add_header("Content-Type: image/jpg");
    else if (url.ends_with("mp4"))
        httpResponse.add_header("Content-Type: video/mp4");
}

auto http::parseResource(http_response &httpResponse, std::string_view range, std::span<const std::byte> body,
                         bool writeBody, std::source_location sourceLocation) -> void {
    constexpr unsigned int maxSize{2097152};

    if (!range.empty()) {
        httpResponse.set_status_code("206 Partial Content");

        range = range.substr(6);
        const auto splitPoint{static_cast<unsigned char>(range.find('-'))};

        const std::string stringStart{range.cbegin(), range.cbegin() + splitPoint};
        const auto digitStart{std::stoul(stringStart)};

        unsigned long digitEnd;
        std::string stringEnd{range.cbegin() + splitPoint + 1, range.cend()};

        if (stringEnd.empty()) {
            digitEnd = digitStart + maxSize - 1;
            if (digitEnd > body.size()) digitEnd = body.size() - 1;

            stringEnd = std::to_string(digitEnd);
        } else
            digitEnd = std::stoul(stringEnd);

        if (digitStart > digitEnd || digitEnd >= body.size()) {
            httpResponse.set_status_code("416 Range Not Satisfiable");
            httpResponse.add_header("Content-Length: 0");
            httpResponse.set_body({});

            throw http_parse_error{<#initializer #>};
        }

        httpResponse.add_header("Content-Range: bytes " + stringStart + '-' + stringEnd + '/' +
                                std::to_string(body.size()));

        body = {body.cbegin() + static_cast<long>(digitStart), body.cbegin() + static_cast<long>(digitEnd) + 1};
    } else if (body.size() > maxSize) {
        httpResponse.set_status_code("206 Partial Content");

        httpResponse.add_header("Content-Range: bytes 0-" + std::to_string(maxSize - 1) + '/' +
                                std::to_string(body.size()));

        body = {body.cbegin(), body.cbegin() + maxSize};
    } else
        httpResponse.set_status_code("200 OK");

    httpResponse.add_header("Content-Length: " + std::to_string(body.size()));
    httpResponse.set_body(writeBody ? body : std::span<const std::byte>{});
}

auto http::parsePost(http_response &httpResponse, std::string_view message, Database &database) -> void {
    httpResponse.set_status_code("200 OK");

    std::array<std::string_view, 4> values;
    for (auto point{values.begin()}; const auto &valueView: std::views::split(message, '&'))
        for (const auto &subValueView: std::views::split(valueView, '=')) *point++ = std::string_view{subValueView};

    if (values[0] == "id") http::parseLogin(httpResponse, values[1], values[3], database);
    else
        http::parseRegister(httpResponse, values[1], database);
}

auto http::parseLogin(http_response &httpResponse, std::string_view id, std::string_view password, Database &database)
        -> void {
    const std::vector<std::vector<std::string>> result{
            database.consult(std::format("select id, password from users where id = {};", id))};
    if (!result.empty()) {
        if (password == result[0][1]) {
            constexpr std::string_view url{"index.html"};

            const std::span<const std::byte> body{http::instance.parseUrl(httpResponse, url)};
            http::parseTypeEncoding(httpResponse, url);

            http::parseResource(httpResponse, "", body, true);
        } else {
            httpResponse.add_header("Content-Type: text/plain; charset=utf-8");

            constexpr std::string_view body{"wrong password"};

            httpResponse.add_header("Content-Length: " + std::to_string(body.size()));
            httpResponse.set_body(std::as_bytes(std::span{body}));
        }
    } else {
        httpResponse.add_header("Content-Type: text/plain; charset=utf-8");

        constexpr std::string_view body{"wrong id"};

        httpResponse.add_header("Content-Length: " + std::to_string(body.size()));
        httpResponse.set_body(std::as_bytes(std::span{body}));
    }
}

auto http::parseRegister(http_response &httpResponse, std::string_view password, Database &database) -> void {
    database.consult(std::format("insert into users (password) values ('{}');", password));

    const std::vector<std::vector<std::string>> result{database.consult("select last_insert_id();")};

    httpResponse.add_header("Content-Type: text/plain; charset=utf-8");

    const std::string body{"id is " + result[0][0]};

    httpResponse.add_header("Content-Length: " + std::to_string(body.size()));
    httpResponse.set_body(std::as_bytes(std::span{body}));
}

const http http::instance;
