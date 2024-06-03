#include "HttpParse.hpp"

#include "../fileDescriptor/Logger.hpp"
#include "../json/JsonValue.hpp"
#include "../log/Exception.hpp"

#include <brotli/encode.h>
#include <cmath>
#include <filesystem>
#include <fstream>

HttpParse::HttpParse(std::shared_ptr<Logger> logger) : logger{std::move(logger)} {
    this->database.connect(std::string_view{}, "AomaYple", "38820233", "webServer", 0, std::string_view{}, 0);
}

auto HttpParse::parse(const std::string_view request, const std::source_location sourceLocation)
    -> std::vector<std::byte> {
    try {
        this->httpRequest = HttpRequest{request};
        this->parseVersion();
    } catch (Exception &exception) {
        this->handleException();
        this->logger->push(std::move(exception.getLog()));
    } catch (std::exception &exception) {
        this->handleException();
        this->logger->push(Log{Log::Level::warn, exception.what(), sourceLocation});
    }

    this->httpResponse.addHeader("Content-Length: " + std::to_string(this->body.size()));

    if (!this->wroteBody) this->body.clear();
    this->httpResponse.setBody(this->body);

    std::vector response{this->httpResponse.toByte()};
    this->clear();

    return response;
}

auto HttpParse::clear() noexcept -> void {
    this->httpRequest = HttpRequest{};
    this->httpResponse = {};
    this->body.clear();
    this->wroteBody = true;
    this->isBrotli = false;
}

auto HttpParse::parseVersion() -> void {
    if (const std::string_view version{this->httpRequest.getVersion()}; version == "HTTP/1.1") {
        this->httpResponse.setVersion(version);

        this->parseMethod();
    } else {
        this->httpResponse.setVersion("HTTP/1.1");
        this->httpResponse.setStatusCode("505 HTTP Version Not Supported");
    }
}

auto HttpParse::parseMethod() -> void {
    if (const std::string_view method{this->httpRequest.getMethod()}; method == "GET" || method == "HEAD") {
        if (method == "HEAD") this->wroteBody = false;

        this->parsePath();
    } else if (method == "POST") {
        this->httpResponse.setStatusCode("200 OK");
        this->httpResponse.addHeader("Content-Type: application/json; charset=utf-8");

        const JsonObject requestBody{this->httpRequest.getBody()};
        const std::string_view password{requestBody["password"]};

        JsonObject jsonBody;
        if (static_cast<std::string_view>(requestBody["method"]) == "login") {
            const std::string_view id{requestBody["id"]};
            const std::vector result{this->database.inquire(
                std::format("select * from users where id = {} and password = '{}';", id, password))};

            jsonBody.add("success", JsonValue{!result.empty()});
        } else {
            this->database.inquire(std::format("insert into users (password) values ('{}');", password));
            std::vector result{this->database.inquire("select last_insert_id();")};

            jsonBody.add("id", JsonValue{std::move(result[0][0])});
        }

        const auto stringBody{jsonBody.toString()};
        const auto spanBody{std::as_bytes(std::span{stringBody})};
        this->body = {spanBody.cbegin(), spanBody.cend()};
    } else this->httpResponse.setStatusCode("405 Method Not Allowed");
}

auto HttpParse::parsePath() -> void {
    const auto url{this->httpRequest.getUrl().substr(1)};
    if (url.empty()) {
        this->httpResponse.setStatusCode("200 OK");
        return;
    }

    std::string_view folder;
    if (url.ends_with("html")) {
        this->httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        this->httpResponse.addHeader("Content-Encoding: br");
        this->isBrotli = true;

        folder = "resources/web";
    } else if (url.ends_with("png")) {
        this->httpResponse.addHeader("Content-Type: image/jpg");

        folder = "resources/images";
    } else if (url.ends_with("ico")) {
        this->httpResponse.addHeader("Content-Type: image/x-icon");

        folder = "resources/images";
    } else if (url.ends_with("mp4")) {
        this->httpResponse.addHeader("Content-Type: video/mp4");

        folder = "resources/videos";
    }

    std::string resourcePath;
    for (const auto &path : std::filesystem::directory_iterator(folder)) {
        if (path.path().filename() == url) {
            resourcePath = path.path().string();

            break;
        }
    }

    if (resourcePath.empty()) this->httpResponse.setStatusCode("404 Not Found");
    else this->parseResource(resourcePath);
}

auto HttpParse::parseResource(const std::string &resourcePath) -> void {
    static constexpr auto maxSize{static_cast<unsigned int>(std::pow(2, 20))};
    const auto resourceSize{static_cast<long>(std::filesystem::file_size(resourcePath))};
    std::pair<long, long> range;

    if (this->httpRequest.containsHeader("Range")) {
        const auto rangeHeader{this->httpRequest.getHeaderValue("Range").substr(6)};
        const unsigned long splitPoint{rangeHeader.find('-')};

        const std::string stringStart{rangeHeader.cbegin(), rangeHeader.cbegin() + splitPoint};
        range.first = std::stol(stringStart);

        std::string stringEnd{rangeHeader.cbegin() + splitPoint + 1, rangeHeader.cend()};
        if (stringEnd.empty()) {
            range.second = range.first + maxSize - 1;
            if (range.second > resourceSize) range.second = resourceSize - 1;

            stringEnd = std::to_string(range.second);
        } else range.second = std::stol(stringEnd);

        if (range.first > range.second || range.second >= resourceSize) {
            this->httpResponse.setStatusCode("416 Range Not Satisfiable");
            this->httpResponse.clearHeaders();

            return;
        }

        this->httpResponse.setStatusCode("206 Partial Content");
        this->httpResponse.addHeader("Content-Range: bytes " + stringStart + '-' + stringEnd + '/' +
                                     std::to_string(resourceSize));
    } else if (resourceSize > maxSize) {
        this->httpResponse.setStatusCode("206 Partial Content");
        this->httpResponse.addHeader("Content-Range: bytes 0-" + std::to_string(maxSize - 1) + '/' +
                                     std::to_string(resourceSize));

        range = {0, maxSize - 1};
    } else [[likely]] {
        this->httpResponse.setStatusCode("200 OK");

        range = {0, resourceSize - 1};
    }

    this->readResource(resourcePath, range);
}

auto HttpParse::readResource(const std::string &resourcePath, const std::pair<long, long> &range,
                             const std::source_location sourceLocation) -> void {
    std::ifstream file{resourcePath, std::ios::binary};
    if (!file) {
        throw Exception{
            Log{Log::Level::error, "cannot open file: " + resourcePath, sourceLocation}
        };
    }

    file.seekg(range.first);

    const long size{range.second - range.first + 1};
    this->body.resize(size);
    if (!file.read(reinterpret_cast<char *>(this->body.data()), size)) {
        throw Exception{
            Log{Log::Level::error, "cannot read file: " + resourcePath, sourceLocation}
        };
    }

    if (this->isBrotli) this->brotli();
}

auto HttpParse::brotli(const std::source_location sourceLocation) -> void {
    unsigned long encodedSize{BrotliEncoderMaxCompressedSize(this->body.size())};
    std::vector<std::byte> encodedBody{encodedSize};

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, this->body.size(),
                              reinterpret_cast<const unsigned char *>(this->body.data()), &encodedSize,
                              reinterpret_cast<unsigned char *>(encodedBody.data())) != BROTLI_TRUE) {
        throw Exception{
            Log{Log::Level::error, "brotli compress failed", sourceLocation}
        };
    }

    encodedBody.resize(encodedSize);
    this->body = std::move(encodedBody);
}

auto HttpParse::handleException() -> void {
    this->httpResponse.setStatusCode("500 Internal Server Error");
    this->httpResponse.clearHeaders();
    this->wroteBody = false;
}
