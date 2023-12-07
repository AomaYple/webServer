#include "http.hpp"

#include "../database/Database.hpp"
#include "../log/Exception.hpp"
#include "../log/logger.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <brotli/encode.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>

[[nodiscard]] auto readFile(std::string_view filepath,
                            std::source_location sourceLocation = std::source_location::current()) noexcept
        -> std::vector<std::byte>;

[[nodiscard]] auto brotli(std::span<const std::byte> data,
                          std::source_location sourceLocation = std::source_location::current()) noexcept
        -> std::vector<std::byte>;

auto parseVersion(HttpResponse &httpResponse, std::string_view version,
                  std::source_location sourceLocation = std::source_location::current()) -> void;

auto parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool isWriteBody) -> void;

[[nodiscard]] auto parseUrl(HttpResponse &httpResponse, std::string_view url,
                            std::source_location sourceLocation = std::source_location::current())
        -> std::span<const std::byte>;

auto parseTypeEncode(HttpResponse &httpResponse, std::string_view url) noexcept -> void;

auto parseResource(HttpResponse &httpResponse, std::string_view range, std::span<const std::byte> body,
                   bool isWriteBody, std::source_location sourceLocation = std::source_location::current()) -> void;

auto parsePost(HttpResponse &httpResponse, std::string_view request, Database &database) -> void;

auto parseLogin(HttpResponse &httpResponse, std::string_view id, std::string_view password, Database &database) -> void;

auto parseRegister(HttpResponse &httpResponse, std::string_view password, Database &database) noexcept -> void;

static const std::unordered_map<std::string, const std::vector<std::byte>> resources{[] noexcept {
    std::unordered_map<std::string, const std::vector<std::byte>> resources;

    resources.emplace("", std::vector<std::byte>{});

    for (const auto &path: std::filesystem::directory_iterator(std::filesystem::current_path().string() + "/resources"))
        for (const auto &subPath: std::filesystem::directory_iterator(path)) {
            std::vector<std::byte> fileContent{readFile(subPath.path().string())};

            auto filename{subPath.path().filename().string()};
            if (filename.ends_with("html")) fileContent = brotli(fileContent);

            resources.emplace(std::move(filename), std::move(fileContent));
        }

    return resources;
}()};

auto http::parse(std::string_view request, Database &database, std::source_location sourceLocation) noexcept
        -> std::vector<std::byte> {
    HttpResponse httpResponse;
    const HttpRequest httpRequest{request};

    try {
        parseVersion(httpResponse, httpRequest.getVersion());

        const std::string_view method{httpRequest.getMethod()};
        if (method == "GET" || method == "HEAD") parseGetHead(httpResponse, httpRequest, method == "GET");
        else if (method == "POST") {
            parsePost(httpResponse, httpRequest.getBody(), database);
        } else {
            httpResponse.setStatusCode("405 Method Not Allowed");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw Exception{Log{Log::Level::warn, "method not allowed: " + std::string{method}, sourceLocation}};
        }
    } catch (Exception &exception) { logger::push(std::move(exception.getLog())); }

    return httpResponse.toBytes();
}

auto readFile(std::string_view filepath, std::source_location sourceLocation) noexcept -> std::vector<std::byte> {
    std::ifstream file{std::string{filepath}};
    if (!file) {
        logger::push(Log{Log::Level::fatal, "cannot open file: " + std::string{filepath}, sourceLocation});
        logger::flush();

        std::terminate();
    }

    std::stringstream stringStream;
    stringStream << file.rdbuf();
    std::string content{stringStream.str()};

    const std::span<const std::byte> contentView{std::as_bytes(std::span{content})};
    return {contentView.cbegin(), contentView.cend()};
}

auto brotli(std::span<const std::byte> data, std::source_location sourceLocation) noexcept -> std::vector<std::byte> {
    unsigned long size{BrotliEncoderMaxCompressedSize(data.size())};

    std::vector<std::byte> buffer(size, std::byte{0});
    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const unsigned char *>(data.data()), &size,
                              reinterpret_cast<unsigned char *>(buffer.data())) != BROTLI_TRUE) {
        logger::push(Log{Log::Level::fatal, "brotli compress failed", sourceLocation});
        logger::flush();

        std::terminate();
    }
    buffer.resize(size);

    return buffer;
}

auto parseVersion(HttpResponse &httpResponse, std::string_view version, std::source_location sourceLocation) -> void {
    if (version != "1.1") {
        httpResponse.setVersion("1.1");
        httpResponse.setStatusCode("505 HTTP Version Not Supported");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{Log{Log::Level::warn, "http version not supported: " + std::string{version}, sourceLocation}};
    }

    httpResponse.setVersion(version);
}

auto parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool isWriteBody) -> void {
    const std::string_view url{httpRequest.getUrl()};

    parseTypeEncode(httpResponse, url);

    const std::span<const std::byte> body{parseUrl(httpResponse, url)};
    parseResource(httpResponse, httpRequest.getHeaderValue("Range"), body, isWriteBody);
}

auto parseUrl(HttpResponse &httpResponse, std::string_view url, std::source_location sourceLocation)
        -> std::span<const std::byte> {
    const auto result{resources.find(std::string{url})};

    if (result == resources.cend()) {
        httpResponse.setStatusCode("404 Not Found");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{Log{Log::Level::warn, "url not found: " + std::string{url}, sourceLocation}};
    }

    return result->second;
}

auto parseTypeEncode(HttpResponse &httpResponse, std::string_view url) noexcept -> void {
    if (url.ends_with("html")) {
        httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("jpg"))
        httpResponse.addHeader("Content-Type: image/jpg");
    else if (url.ends_with("mp4"))
        httpResponse.addHeader("Content-Type: video/mp4");
}

auto parseResource(HttpResponse &httpResponse, std::string_view range, std::span<const std::byte> body,
                   bool isWriteBody, std::source_location sourceLocation) -> void {
    static constexpr unsigned int maxSize{1048576};

    if (!range.empty()) {
        httpResponse.setStatusCode("206 Partial Content");

        range = range.substr(6);
        const unsigned long splitPoint{range.find('-')};

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
            httpResponse.setStatusCode("416 Range Not Satisfiable");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw Exception{Log{Log::Level::warn, "range not satisfiable: " + std::string{range}, sourceLocation}};
        }

        httpResponse.addHeader("Content-Range: bytes " + stringStart + '-' + stringEnd + '/' +
                               std::to_string(body.size()));

        body = {body.cbegin() + static_cast<long>(digitStart), body.cbegin() + static_cast<long>(digitEnd) + 1};
    } else if (body.size() > maxSize) {
        httpResponse.setStatusCode("206 Partial Content");

        httpResponse.addHeader("Content-Range: bytes 0-" + std::to_string(maxSize - 1) + '/' +
                               std::to_string(body.size()));

        body = {body.cbegin(), body.cbegin() + maxSize};
    } else
        httpResponse.setStatusCode("200 OK");

    httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));

    httpResponse.setBody(isWriteBody ? body : std::span<const std::byte>{});
}

auto parsePost(HttpResponse &httpResponse, std::string_view request, Database &database) -> void {
    httpResponse.setStatusCode("200 OK");

    std::array<std::string_view, 4> values;
    for (auto point{values.begin()}; const auto &valueView: std::views::split(request, '&'))
        for (const auto &subValueView: std::views::split(valueView, '=')) *point++ = std::string_view{subValueView};

    if (values[0] == "id") parseLogin(httpResponse, values[1], values[3], database);
    else
        parseRegister(httpResponse, values[1], database);
}

auto parseLogin(HttpResponse &httpResponse, std::string_view id, std::string_view password, Database &database)
        -> void {
    const std::vector<std::vector<std::string>> result{
            database.consult(std::format("select * from users where id = {} and password = '{}';", id, password))};
    if (result.empty()) {
        httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

        static constexpr std::string_view body{"wrong id or password"};
        httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));
        httpResponse.setBody(std::as_bytes(std::span{body}));
    } else {
        static constexpr std::string_view url{"index.html"};

        const std::span<const std::byte> body{parseUrl(httpResponse, url)};
        parseTypeEncode(httpResponse, url);
        parseResource(httpResponse, "", body, true);
    }
}

auto parseRegister(HttpResponse &httpResponse, std::string_view password, Database &database) noexcept -> void {
    database.consult(std::format("insert into users (password) values ('{}');", password));

    const std::vector<std::vector<std::string>> result{database.consult("select last_insert_id();")};

    httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

    const std::string body{"id is " + result[0][0]};
    httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));
    httpResponse.setBody(std::as_bytes(std::span{body}));
}
