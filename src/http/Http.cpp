#include "Http.h"

#include "../database/Database.h"
#include "../exception/Exception.h"
#include "../log/Log.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <brotli/encode.h>

#include <filesystem>
#include <format>
#include <ranges>

using namespace std;

Http::Http()
    : resources{[] {
          unordered_map<string, vector<byte>> tempResources;

          tempResources.emplace("", vector<byte>{});

          for (const auto &path: filesystem::directory_iterator(filesystem::current_path().string() + "/web")) {
              string filename{path.path().filename().string()};

              vector<byte> fileContent{Http::readFile(path.path().string())};

              if (filename.ends_with("html")) fileContent = Http::brotli(fileContent);

              tempResources.emplace(std::move(filename), std::move(fileContent));
          }

          return tempResources;
      }()} {}

auto Http::readFile(string_view filepath, source_location sourceLocation) -> vector<byte> {
    ifstream file{string{filepath}, ios::binary | ios::ate};
    if (!file)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "file not found: " + string{filepath})};

    const auto size{file.tellg()};
    file.seekg(0, ios::beg);

    vector<byte> buffer((size * sizeof(char)) / sizeof(byte), byte{0});

    file.read(reinterpret_cast<char *>(buffer.data()), size);
    if (file.gcount() != size)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "file read error: " + string{filepath})};

    return buffer;
}

auto Http::brotli(span<const byte> data, source_location sourceLocation) -> vector<byte> {
    size_t size{BrotliEncoderMaxCompressedSize(data.size())};

    vector<byte> buffer((size * sizeof(uint8_t)) / sizeof(byte), byte{0});

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const uint8_t *>(data.data()), &size,
                              reinterpret_cast<uint8_t *>(buffer.data())) != BROTLI_TRUE)
        throw Exception{Log::formatLog(Log::Level::Fatal, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "brotli compression error")};

    buffer.resize((size * sizeof(uint8_t)) / sizeof(byte));

    return buffer;
}

auto Http::parse(string_view request, Database &database) -> vector<byte> {
    HttpResponse httpResponse;

    const HttpRequest httpRequest{HttpRequest::parse(request)};

    try {
        Http::parseVersion(httpResponse, httpRequest.getVersion());

        const string_view method{httpRequest.getMethod()};

        if (method == "GET" || method == "HEAD") Http::parseGetHead(httpResponse, httpRequest, method == "GET");
        else if (method == "POST") {
            Http::parsePost(httpResponse, httpRequest.getBody(), database);
        } else {
            httpResponse.setStatusCode("405 Method Not Allowed");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw Exception{Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                           source_location::current(), "unsupported HTTP method: " + string{method})};
        }
    } catch (const Exception &exception) { Log::produce(exception.what()); }

    return httpResponse.combine();
}

auto Http::parseVersion(HttpResponse &httpResponse, string_view version, source_location sourceLocation) -> void {
    if (version != "1.1") {
        httpResponse.setVersion("1.1");
        httpResponse.setStatusCode("505 HTTP Version Not Supported");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "unsupported HTTP version: " + string{version})};
    }

    httpResponse.setVersion(version);
}

auto Http::parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody) -> void {
    const string_view url{httpRequest.getUrl()};

    const span<const byte> body{Http::instance.parseUrl(httpResponse, url)};
    Http::parseTypeEncoding(httpResponse, url);

    Http::parseResource(httpResponse, httpRequest.getHeaderValue("Range"), body, writeBody);
}

auto Http::parseUrl(HttpResponse &httpResponse, string_view url, source_location sourceLocation) const
        -> span<const byte> {
    const auto result{this->resources.find(string{url})};

    if (result == this->resources.end()) {
        httpResponse.setStatusCode("404 Not Found");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                       sourceLocation, "resource not found: " + string{url})};
    }

    return result->second;
}

auto Http::parseTypeEncoding(HttpResponse &httpResponse, string_view url) -> void {
    if (url.ends_with("html")) {
        httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("jpg"))
        httpResponse.addHeader("Content-Type: image/jpg");
    else if (url.ends_with("mp4"))
        httpResponse.addHeader("Content-Type: video/mp4");
}

auto Http::parseResource(HttpResponse &httpResponse, string_view range, span<const byte> body, bool writeBody,
                         source_location sourceLocation) -> void {
    constexpr unsigned int maxSize{2097152};

    if (!range.empty()) {
        httpResponse.setStatusCode("206 Partial Content");

        string_view point{"="};
        const auto splitPoint{ranges::search(range, point)};

        point = "-";
        const auto secondSplitPoint{ranges::search(range, point)};

        const string stringStart{splitPoint.begin() + 1, secondSplitPoint.begin()};
        unsigned long digitStart{stoul(stringStart)}, digitEnd;

        string stringEnd{secondSplitPoint.begin() + 1, range.end()};
        if (stringEnd.empty()) {
            digitEnd = digitStart + maxSize - 1;
            if (digitEnd > body.size()) digitEnd = body.size() - 1;

            stringEnd = to_string(digitEnd);
        } else
            digitEnd = stoul(stringEnd);

        if (digitStart > digitEnd || digitEnd >= body.size()) {
            httpResponse.setStatusCode("416 Range Not Satisfiable");
            httpResponse.addHeader("Content-Length: 0");
            httpResponse.setBody({});

            throw Exception{Log::formatLog(Log::Level::Warn, chrono::system_clock::now(), this_thread::get_id(),
                                           sourceLocation, "range not satisfiable: " + string{range})};
        }

        httpResponse.addHeader("Content-Range: bytes " + stringStart + "-" + stringEnd + "/" + to_string(body.size()));

        body = {body.begin() + static_cast<long>(digitStart), body.begin() + static_cast<long>(digitEnd + 1)};
    } else if (body.size() > maxSize) {
        httpResponse.setStatusCode("206 Partial Content");

        httpResponse.addHeader("Content-Range: bytes 0-" + to_string(maxSize - 1) + "/" + to_string(body.size()));

        body = {body.begin(), body.begin() + maxSize};
    } else
        httpResponse.setStatusCode("200 OK");

    httpResponse.addHeader("Content-Length: " + to_string(body.size()));
    httpResponse.setBody(writeBody ? body : span<const byte>{});
}

auto Http::parsePost(HttpResponse &httpResponse, string_view message, Database &database) -> void {
    httpResponse.setStatusCode("200 OK");

    array<string_view, 4> values;
    for (auto point{values.begin()}; const auto &valueView: views::split(message, '&'))
        for (const auto &subValueView: views::split(valueView, '=')) *point++ = string_view{subValueView};

    if (values[0] == "id") Http::parseLogin(httpResponse, values[1], values[3], database);
    else
        Http::parseRegister(httpResponse, values[1], database);
}

auto Http::parseLogin(HttpResponse &httpResponse, string_view id, string_view password, Database &database) -> void {
    const vector<vector<string>> result{database.consult(format("select id, password from users where id = {};", id))};
    if (!result.empty()) {
        if (password == result[0][1]) {
            constexpr string_view url{"index.html"};

            const span<const byte> body{Http::instance.parseUrl(httpResponse, url)};
            Http::parseTypeEncoding(httpResponse, url);

            Http::parseResource(httpResponse, "", body, true);
        } else {
            httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

            constexpr string_view body{"wrong password"};

            httpResponse.addHeader("Content-Length: " + to_string(body.size()));
            httpResponse.setBody(as_bytes(span{body}));
        }
    } else {
        httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

        constexpr string_view body{"wrong id"};

        httpResponse.addHeader("Content-Length: " + to_string(body.size()));
        httpResponse.setBody(as_bytes(span{body}));
    }
}

auto Http::parseRegister(HttpResponse &httpResponse, string_view password, Database &database) -> void {
    database.consult(format("insert into users (password) values ('{}');", password));

    const vector<vector<string>> result{database.consult("select last_insert_id();")};

    httpResponse.addHeader("Content-Type: text/plain; charset=utf-8");

    const string body{"id is " + result[0][0]};

    httpResponse.addHeader("Content-Length: " + to_string(body.size()));
    httpResponse.setBody(as_bytes(span{body}));
}

const Http Http::instance;
