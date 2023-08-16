#include "Http.h"

#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <brotli/encode.h>

#include <filesystem>

using std::hex, std::to_string, std::filesystem::directory_iterator, std::filesystem::current_path, std::ranges::search,
        std::this_thread::get_id;
using std::ios;
using std::queue, std::span, std::vector, std::byte, std::ifstream, std::ostringstream, std::source_location,
        std::string, std::string_view, std::unordered_map;
using std::chrono::system_clock;

Http::Http()
    : resources{[] {
          unordered_map<string, vector<byte>> tempResources;

          tempResources.emplace("", vector<byte>{});

          for (const auto &path: directory_iterator(current_path().string() + "/web")) {
              string filename{path.path().filename().string()};

              vector<byte> fileContent{Http::readFile(path.path().string())};

              if (filename.ends_with("html")) { fileContent = Http::brotli(fileContent); }

              tempResources.emplace(std::move(filename), std::move(fileContent));
          }

          return tempResources;
      }()} {}

auto Http::readFile(string_view filepath, source_location sourceLocation) -> vector<byte> {
    ifstream file{string{filepath}, ios::binary};
    if (!file)
        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL,
                                         "file not found: " + string{filepath})};

    file.seekg(0, ios::end);
    auto size{file.tellg()};
    file.seekg(0, ios::beg);

    vector<byte> buffer(size * sizeof(char), byte{0});

    file.read(reinterpret_cast<char *>(buffer.data()), size);

    return buffer;
}

auto Http::brotli(span<const byte> data, source_location sourceLocation) -> vector<byte> {
    size_t length{BrotliEncoderMaxCompressedSize(data.size_bytes())};

    vector<byte> buffer(length, byte{0});

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size_bytes(),
                              reinterpret_cast<const uint8_t *>(data.data()), &length,
                              reinterpret_cast<uint8_t *>(buffer.data())) != BROTLI_TRUE)
        throw Exception{
                message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL, "brotli compress error")};

    buffer.resize(length);

    return buffer;
}

auto Http::parse(span<const byte> request, Database &database) -> queue<vector<byte>> {
    queue<vector<byte>> content, subContent;

    const HttpRequest httpRequest{HttpRequest::parse(string_view{reinterpret_cast<const char *>(request.data())})};
    HttpResponse httpResponse;

    try {
        subContent = Http::parseVersion(httpResponse, httpRequest);
    } catch (const Exception &exception) { Log::produce(exception.what()); }

    content.emplace();
    content.emplace(httpResponse.combine());
    while (!subContent.empty()) {
        content.emplace(std::move(subContent.front()));

        subContent.pop();
    }

    return content;
}

auto Http::parseVersion(HttpResponse &httpResponse, const HttpRequest &httpRequest, source_location sourceLocation)
        -> queue<vector<byte>> {
    queue<vector<byte>> content;
    const string_view version{httpRequest.getVersion()};

    if (version == "1.1") {
        httpResponse.setVersion(version);

        content = Http::parseMethod(httpResponse, httpRequest);
    } else {
        httpResponse.setVersion("1.1");
        httpResponse.setStatusCode("505 HTTP Version Not Supported");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL,
                                         "unsupported HTTP version: " + string{version})};
    }

    return content;
}

auto Http::parseMethod(HttpResponse &httpResponse, const HttpRequest &httpRequest, source_location sourceLocation)
        -> queue<vector<byte>> {
    queue<vector<byte>> content;
    const string_view method{httpRequest.getMethod()};

    if (method == "GET") content = Http::instance.parseUrl(httpResponse, httpRequest, true);
    else if (method == "HEAD")
        content = Http::instance.parseUrl(httpResponse, httpRequest, false);
    else if (method == "POST") {

    } else {
        httpResponse.setStatusCode("405 Method Not Allowed");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL,
                                         "unsupported HTTP method: " + string{method})};
    }

    return content;
}

auto Http::parseUrl(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody,
                    std::source_location sourceLocation) const -> queue<vector<byte>> {
    queue<vector<byte>> content;
    const string url{httpRequest.getUrl()};

    const auto result{this->resources.find(url)};
    if (result != this->resources.end()) {
        Http::parseTypeEncoding(url, httpResponse);

        content = Http::parseResource(httpResponse, httpRequest, result->second, writeBody);
    } else {
        httpResponse.setStatusCode("404 Not Found");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::FATAL,
                                         "resource not found: " + url)};
    }

    return content;
}

auto Http::parseTypeEncoding(string_view url, HttpResponse &httpResponse) -> void {
    if (url.ends_with("html")) {
        httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("png"))
        httpResponse.addHeader("Content-Type: image/png");
    else if (url.ends_with("mp4"))
        httpResponse.addHeader("Content-Type: video/mp4");
}

auto Http::parseResource(HttpResponse &httpResponse, const HttpRequest &httpRequest, span<const byte> body,
                         bool writeBody, source_location sourceLocation) -> queue<vector<byte>> {
    queue<vector<byte>> content;
    constexpr std::uint_least32_t maxSize{2097152};
    const string_view range{httpRequest.getHeaderValue("Range")};

    if (!range.empty()) {
        Http::parseRange(httpResponse, maxSize, range, body, writeBody, sourceLocation);
    } else if (body.size() > maxSize) {
        content = Http::parseOversize(httpResponse, maxSize, body, writeBody);
    } else
        Http::parseNormal(httpResponse, body, writeBody);

    return content;
}

auto Http::parseRange(HttpResponse &httpResponse, std::uint_least32_t maxSize, string_view range, span<const byte> body,
                      bool writeBody, source_location sourceLocation) -> void {
    httpResponse.setStatusCode("206 Partial Content");

    string_view point{"="};
    const auto splitPoint{search(range, point)};

    point = "-";
    const auto secondSplitPoint{search(range, point)};

    const string stringStart{splitPoint.begin() + 1, secondSplitPoint.begin()};
    long digitStart{stol(stringStart)}, digitEnd;

    string stringEnd{secondSplitPoint.begin() + 1, range.end()};
    if (stringEnd.empty()) {
        digitEnd = digitStart + maxSize - 1;
        stringEnd = to_string(digitEnd);
    } else
        digitEnd = stol(stringEnd);

    if (digitStart < 0 || digitEnd < 0 || digitStart > digitEnd || digitEnd >= body.size()) {
        httpResponse.setStatusCode("416 Range Not Satisfiable");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(system_clock::now(), get_id(), sourceLocation, Level::WARN,
                                         "invalid range: " + string{range})};
    }

    httpResponse.addHeader("Content-Range: bytes " + stringStart + "-" + stringEnd + "/" + to_string(body.size()));

    body = {body.begin() + digitStart, body.begin() + digitEnd - digitStart + 1};

    httpResponse.addHeader("Content-Length: " + to_string(body.size()));

    httpResponse.setBody(writeBody ? body : span<const byte>{});
}

auto Http::parseOversize(HttpResponse &httpResponse, std::uint_least32_t maxSize, span<const byte> body, bool writeBody)
        -> queue<vector<byte>> {
    queue<vector<byte>> content;

    httpResponse.setStatusCode("200 OK");
    httpResponse.addHeader("Transfer-Encoding: chunked");

    if (writeBody) {
        const auto conversions{[](std::int_fast64_t number) {
            const string stringNumber{Http::decimalToHexadecimal(number)};
            const span<const byte> spanNumber{as_bytes(span{stringNumber})};

            return vector<byte>{spanNumber.begin(), spanNumber.end()};
        }};

        vector<byte> hexadecimalSize{conversions(maxSize)};
        vector<byte> chunk;
        const span<const byte> separator{as_bytes(span{"\r\n"})};
        separator.subspan(0, separator.size() - 1);
        auto point{body.begin()};

        while (point <= body.end()) {
            chunk.clear();

            chunk.insert(chunk.end(), hexadecimalSize.begin(), hexadecimalSize.end());
            chunk.insert(chunk.end(), separator.begin(), separator.end());
            chunk.insert(chunk.end(), point, point + maxSize);
            chunk.insert(chunk.end(), separator.begin(), separator.end());
            content.emplace(std::move(chunk));

            point += maxSize;
        }

        if (point != body.end()) {
            chunk.clear();
            hexadecimalSize = conversions(body.end() - point);

            chunk.insert(chunk.end(), hexadecimalSize.begin(), hexadecimalSize.end());
            chunk.insert(chunk.end(), separator.begin(), separator.end());
            chunk.insert(chunk.end(), point, body.end());
            chunk.insert(chunk.end(), separator.begin(), separator.end());

            content.emplace(std::move(chunk));
        }

        point = body.end();
        chunk.clear();
        hexadecimalSize = conversions(body.end() - point);

        chunk.insert(chunk.end(), hexadecimalSize.begin(), hexadecimalSize.end());
        chunk.insert(chunk.end(), separator.begin(), separator.end());
        chunk.insert(chunk.end(), point, body.end());
        chunk.insert(chunk.end(), separator.begin(), separator.end());

        content.emplace(std::move(chunk));
    }

    return content;
}

auto Http::decimalToHexadecimal(std::int_fast64_t number) -> string {
    ostringstream stream;

    if (number < 0) stream << "-";

    stream << hex << std::abs(number);

    return stream.str();
}

auto Http::parseNormal(HttpResponse &httpResponse, span<const byte> body, bool writeBody) -> void {
    httpResponse.setStatusCode("200 OK");
    httpResponse.addHeader("Content-Length: " + to_string(body.size()));

    httpResponse.setBody(writeBody ? body : span<const byte>{});
}

const Http Http::instance;
