#include "Http.hpp"

#include "../database/Database.hpp"
#include "../log/Log.hpp"
#include "HttpParseError.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <brotli/encode.h>

#include <filesystem>
#include <format>
#include <ranges>

Http::Http()
    : resources{[] {
          std::unordered_map<std::string, std::vector<std::byte>> tempResources;

          tempResources.emplace("", std::vector<std::byte>{});

          for (const auto &path:
               std::filesystem::directory_iterator(std::filesystem::current_path().string() + "/web")) {
              auto filename{path.path().filename().string()};

              std::vector<std::byte> fileContent{Http::readFile(path.path().string())};

              if (filename.ends_with("html")) fileContent = Http::brotli(fileContent);

              tempResources.emplace(std::move(filename), std::move(fileContent));
          }

          return tempResources;
      }()} {}

auto Http::readFile(std::string_view filepath, std::source_location sourceLocation) -> std::vector<std::byte> {
    std::ifstream file{std::string{filepath}, std::ios::binary | std::ios::ate};
    if (!file)
        throw HttpParseError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                            std::this_thread::get_id(), sourceLocation,
                                            "file not found: " + std::string{filepath})};

    const auto size{static_cast<unsigned long>(file.tellg())};
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(size, std::byte{0});

    file.read(reinterpret_cast<char *>(buffer.data()), static_cast<long>(size));
    if (file.gcount() != static_cast<long>(size))
        throw HttpParseError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                            std::this_thread::get_id(), sourceLocation,
                                            "file read error: " + std::string{filepath})};

    return buffer;
}

auto Http::brotli(std::span<const std::byte> data, std::source_location sourceLocation) -> std::vector<std::byte> {
    unsigned long size{BrotliEncoderMaxCompressedSize(data.size())};

    std::vector<std::byte> buffer(size, std::byte{0});

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const unsigned char *>(data.data()), &size,
                              reinterpret_cast<unsigned char *>(buffer.data())) != BROTLI_TRUE)
        throw HttpParseError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                            std::this_thread::get_id(), sourceLocation, "brotli compression error")};

    buffer.resize(size);

    return buffer;
}

auto Http::parse(std::string_view request, Database &database) -> std::vector<std::byte> {
    HttpResponse httpResponse;

    const HttpRequest httpRequest{request};

    try {
        Http::parseVersion(httpResponse, httpRequest.getVersion());

        const std::string_view method{httpRequest.getMethod()};

        if (method == "GET" || method == "HEAD") Http::parseGetHead(httpResponse, httpRequest, method == "GET");
        else if (method == "POST") {
            Http::parsePost(httpResponse, httpRequest.getBody(), database);
        } else {
            httpResponse.setStatusCode("405 Method Not Allowed");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw HttpParseError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                                std::this_thread::get_id(), std::source_location::current(),
                                                "unsupported HTTP method: " + std::string{method})};
        }
    } catch (const HttpParseError &httpParseError) { Log::produce(httpParseError.what()); }

    return httpResponse.combine();
}

auto Http::parseVersion(HttpResponse &httpResponse, std::string_view version, std::source_location sourceLocation)
        -> void {
    if (version != "1.1") {
        httpResponse.setVersion("1.1");
        httpResponse.setStatusCode("505 HTTP Version Not Supported");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw HttpParseError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                            std::this_thread::get_id(), sourceLocation,
                                            "unsupported HTTP version: " + std::string{version})};
    }

    httpResponse.setVersion(version);
}

auto Http::parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody) -> void {
    const std::string_view url{httpRequest.getUrl()};

    Http::parseTypeEncoding(httpResponse, url);

    const std::span<const std::byte> body{Http::instance.parseUrl(httpResponse, url)};
    Http::parseResource(httpResponse, httpRequest.getHeaderValue("Range"), body, writeBody);
}

auto Http::parseUrl(HttpResponse &httpResponse, std::string_view url, std::source_location sourceLocation) const
        -> std::span<const std::byte> {
    const auto result{this->resources.find(std::string{url})};

    if (result == this->resources.cend()) {
        httpResponse.setStatusCode("404 Not Found");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw HttpParseError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                            std::this_thread::get_id(), sourceLocation,
                                            "resource not found: " + std::string{url})};
    }

    return result->second;
}

auto Http::parseTypeEncoding(HttpResponse &httpResponse, std::string_view url) -> void {
    if (url.ends_with("html")) {
        httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("jpg"))
        httpResponse.addHeader("Content-Type: image/jpg");
    else if (url.ends_with("mp4"))
        httpResponse.addHeader("Content-Type: video/mp4");
}

auto Http::parseResource(HttpResponse &httpResponse, std::string_view range, std::span<const std::byte> body,
                         bool writeBody, std::source_location sourceLocation) -> void {
    constexpr unsigned int maxSize{2097152};

    if (!range.empty()) {
        httpResponse.setStatusCode("206 Partial Content");

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
            httpResponse.setStatusCode("416 Range Not Satisfiable");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw HttpParseError{Log::formatLog(Log::Level::Warn, std::chrono::system_clock::now(),
                                                std::this_thread::get_id(), sourceLocation,
                                                "range not satisfiable: " + std::string{range})};
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
    httpResponse.setBody(writeBody ? body : std::span<const std::byte>{});
}

auto Http::parsePost(HttpResponse &httpResponse, std::string_view message, Database &database) -> void {
    httpResponse.setStatusCode("200 OK");

    std::array<std::string_view, 4> values;
    for (auto point{values.begin()}; const auto &valueView: std::views::split(message, '&'))
        for (const auto &subValueView: std::views::split(valueView, '=')) *point++ = std::string_view{subValueView};

    if (values[0] == "id") Http::parseLogin(httpResponse, values[1], values[3], database);
    else
        Http::parseRegister(httpResponse, values[1], database);
}

auto Http::parseLogin(HttpResponse &httpResponse, std::string_view id, std::string_view password, Database &database)
        -> void {
    const std::vector<std::vector<std::string>> result{
            database.consult(std::format("select id, password from users where id = {};", id))};
    if (!result.empty()) {
        if (password == result[0][1]) {
            constexpr std::string_view url{"index.html"};

            const std::span<const std::byte> body{Http::instance.parseUrl(httpResponse, url)};
            Http::parseTypeEncoding(httpResponse, url);

            Http::parseResource(httpResponse, "", body, true);
        } else {
            httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

            constexpr std::string_view body{"wrong password"};

            httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));
            httpResponse.setBody(std::as_bytes(std::span{body}));
        }
    } else {
        httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

        constexpr std::string_view body{"wrong id"};

        httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));
        httpResponse.setBody(std::as_bytes(std::span{body}));
    }
}

auto Http::parseRegister(HttpResponse &httpResponse, std::string_view password, Database &database) -> void {
    database.consult(std::format("insert into users (password) values ('{}');", password));

    const std::vector<std::vector<std::string>> result{database.consult("select last_insert_id();")};

    httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

    const std::string body{"id is " + result[0][0]};

    httpResponse.addHeader("Content-Length: " + std::to_string(body.size()));
    httpResponse.setBody(std::as_bytes(std::span{body}));
}

const Http Http::instance;
