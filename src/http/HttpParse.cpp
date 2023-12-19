#include "HttpParse.hpp"

#include "../json/JsonValue.hpp"
#include "../log/logger.hpp"

#include <brotli/encode.h>

#include <filesystem>
#include <format>
#include <fstream>

HttpParse::HttpParse(Database &&database) noexcept : database{std::move(database)} {}

auto HttpParse::parse(std::string_view request) -> std::vector<std::byte> {
    this->httpRequest = HttpRequest{request};

    try {
        this->parseVersion();
    } catch (Exception &exception) {
        this->httpResponse.setStatusCode("500 Internal Server Error");
        this->httpResponse.clearHeaders();
        this->isWriteBody = false;

        logger::push(std::move(exception.getLog()));
    }

    this->httpResponse.addHeader("Content-Length: " + std::to_string(this->body.size()));
    if (!this->isWriteBody) this->body.clear();
    this->httpResponse.setBody(this->body);

    std::vector<std::byte> response{this->httpResponse.toBytes()};
    this->clear();

    return response;
}

auto HttpParse::clear() noexcept -> void {
    this->httpRequest = HttpRequest{""};
    this->httpResponse = HttpResponse{};
    this->body.clear();
    this->isWriteBody = true;
    this->isBrotli = false;
}

auto HttpParse::parseVersion() -> void {
    const std::string_view version{this->httpRequest.getVersion()};
    if (version != "HTTP/1.1") {
        this->httpResponse.setVersion("HTTP/1.1");
        this->httpResponse.setStatusCode("505 HTTP Version Not Supported");
    } else {
        this->httpResponse.setVersion(version);

        this->parseMethod();
    }
}

auto HttpParse::parseMethod() -> void {
    const std::string_view method{this->httpRequest.getMethod()};
    if (method == "GET" || method == "HEAD") {
        if (method == "HEAD") this->isWriteBody = false;

        this->parsePath();
    } else if (method == "POST") {
        this->httpResponse.setStatusCode("200 OK");
        this->httpResponse.addHeader("Content-Type: application/json; charset=utf-8");

        const JsonObject requestBody{this->httpRequest.getBody()};
        const std::string_view password{requestBody["password"].getString()};

        JsonObject jsonBody;
        if (requestBody["method"].getString() == "login") {
            const std::string_view id{requestBody["id"].getString()};
            const std::vector<std::vector<std::string>> result{this->database.inquire(
                    std::format("select * from users where id = {} and password = '{}';", id, password))};

            jsonBody.add("success", JsonValue{!result.empty()});
        } else {
            this->database.inquire(std::format("insert into users (password) values ('{}');", password));
            std::vector<std::vector<std::string>> result{this->database.inquire("select last_insert_id();")};

            jsonBody.add("id", JsonValue{std::move(result[0][0])});
        }

        const std::string stringBody{jsonBody.toString()};
        const std::span<const std::byte> spanBody{std::as_bytes(std::span{stringBody})};
        this->body = {spanBody.cbegin(), spanBody.cend()};
    } else
        this->httpResponse.setStatusCode("405 Method Not Allowed");
}

auto HttpParse::parsePath() -> void {
    const std::string_view url{this->httpRequest.getUrl().substr(1)};
    if (url.empty()) {
        this->httpResponse.setStatusCode("200 OK");
        return;
    }

    std::string resourcePath;
    for (const auto &path:
         std::filesystem::recursive_directory_iterator{std::filesystem::current_path().string() + "/resources"})
        if (path.is_regular_file() && path.path().filename() == url) resourcePath = path.path().string();

    if (resourcePath.empty()) this->httpResponse.setStatusCode("404 Not Found");
    else
        this->parseType(resourcePath);
}

auto HttpParse::parseType(const std::string &resourcePath) -> void {
    if (resourcePath.ends_with("html")) {
        this->httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        this->httpResponse.addHeader("Content-Encoding: br");
        this->isBrotli = true;
    } else if (resourcePath.ends_with("jpg"))
        this->httpResponse.addHeader("Content-Type: image/jpg");
    else if (resourcePath.ends_with("ico"))
        this->httpResponse.addHeader("Content-Type: image/x-icon");
    else if (resourcePath.ends_with("mp4"))
        this->httpResponse.addHeader("Content-Type: video/mp4");

    this->parseResource(resourcePath);
}

auto HttpParse::parseResource(const std::string &resourcePath) -> void {
    static constexpr unsigned int maxSize{1048576};
    const long resourceSize{static_cast<long>(std::filesystem::file_size(resourcePath))};
    std::pair<long, long> range;

    if (this->httpRequest.containsHeader("Range")) {
        const std::string_view rangeHeader{this->httpRequest.getHeaderValue("Range").substr(6)};
        const unsigned long splitPoint{rangeHeader.find('-')};

        const std::string stringStart{rangeHeader.cbegin(), rangeHeader.cbegin() + splitPoint};
        range.first = std::stol(stringStart);

        std::string stringEnd{rangeHeader.cbegin() + splitPoint + 1, rangeHeader.cend()};
        if (stringEnd.empty()) {
            range.second = range.first + maxSize - 1;
            if (range.second > resourceSize) range.second = resourceSize - 1;

            stringEnd = std::to_string(range.second);
        } else
            range.second = std::stol(stringEnd);

        if (range.first > range.second || range.second >= resourceSize) {
            this->httpResponse.setStatusCode("416 Range Not Satisfiable");
            this->httpResponse.clearHeaders();

            return;
        } else {
            this->httpResponse.setStatusCode("206 Partial Content");
            this->httpResponse.addHeader("Content-Range: bytes " + stringStart + '-' + stringEnd + '/' +
                                         std::to_string(resourceSize));
        }
    } else if (resourceSize > maxSize) {
        this->httpResponse.setStatusCode("206 Partial Content");
        this->httpResponse.addHeader("Content-Range: bytes 0-" + std::to_string(maxSize - 1) + '/' +
                                     std::to_string(resourceSize));

        range = {0, maxSize - 1};
    } else {
        this->httpResponse.setStatusCode("200 OK");

        range = {0, resourceSize - 1};
    }

    this->readResource(resourcePath, range);
}

auto HttpParse::readResource(const std::string &resourcePath, std::pair<long, long> range,
                             std::source_location sourceLocation) -> void {
    std::ifstream file{resourcePath, std::ios::binary};
    if (!file) throw Exception{Log{Log::Level::error, "cannot open file: " + resourcePath, sourceLocation}};

    file.seekg(range.first);

    const long size{range.second - range.first + 1};
    this->body.resize(size, std::byte{0});
    if (!file.read(reinterpret_cast<char *>(this->body.data()), size))
        throw Exception{Log{Log::Level::error, "cannot read file: " + resourcePath, sourceLocation}};

    if (this->isBrotli) this->brotli();
}

auto HttpParse::brotli(std::source_location sourceLocation) -> void {
    unsigned long encodedSize{BrotliEncoderMaxCompressedSize(this->body.size())};

    std::vector<std::byte> encodedBody(encodedSize, std::byte{0});
    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, this->body.size(),
                              reinterpret_cast<const unsigned char *>(this->body.data()), &encodedSize,
                              reinterpret_cast<unsigned char *>(encodedBody.data())) != BROTLI_TRUE)
        throw Exception{Log{Log::Level::error, "brotli compress failed", sourceLocation}};

    encodedBody.resize(encodedSize);
    this->body = std::move(encodedBody);
}
