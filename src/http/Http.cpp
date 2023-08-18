#include "Http.h"

#include "../exception/Exception.h"
#include "../log/Log.h"
#include "../log/message.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <brotli/encode.h>

#include <filesystem>
#include <unordered_set>

using namespace std;

Http::Http()
    : resources{[] {
          unordered_map<string, vector<byte>> tempResources;

          tempResources.emplace("", vector<byte>{});

          for (auto const &path: filesystem::directory_iterator(filesystem::current_path().string() + "/web")) {
              string filename{path.path().filename().string()};

              vector<byte> fileContent{Http::readFile(path.path().string())};

              if (filename.ends_with("html") || filename.ends_with("css") || filename.ends_with("js"))
                  fileContent = Http::brotli(fileContent);

              tempResources.emplace(std::move(filename), std::move(fileContent));
          }

          return tempResources;
      }()} {}

auto Http::readFile(string_view filepath, source_location sourceLocation) -> vector<byte> {
    ifstream file{string{filepath}, ios::binary};
    if (!file)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "file not found: " + string{filepath})};

    file.seekg(0, ios::end);
    auto size{file.tellg()};
    file.seekg(0, ios::beg);

    vector<byte> buffer(size, byte{0});

    file.read(reinterpret_cast<char *>(buffer.data()), size);

    return buffer;
}

auto Http::brotli(span<const byte> data, source_location sourceLocation) -> vector<byte> {
    unsigned long size{BrotliEncoderMaxCompressedSize(data.size())};

    vector<byte> buffer(size, byte{0});

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const unsigned char *>(data.data()), &size,
                              reinterpret_cast<unsigned char *>(buffer.data())) != BROTLI_TRUE)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "brotli compress error")};

    buffer.resize(size);

    return buffer;
}

auto Http::parse(span<const byte> request, Database &database) -> vector<byte> {
    HttpResponse httpResponse;

    const HttpRequest httpRequest{HttpRequest::parse(reinterpret_cast<const char *>(request.data()))};

    try {
        const string_view method{httpRequest.getMethod()};
        Http::parseMethod(httpResponse, method);

        if (method == "GET" || method == "HEAD") Http::parseGetHead(httpResponse, httpRequest, method == "GET");
        else {}
    } catch (const Exception &exception) { Log::produce(exception.what()); }

    return httpResponse.combine();
}

auto Http::parseMethod(HttpResponse &httpResponse, string_view method, source_location sourceLocation) -> void {
    const unordered_set<string_view> methods{"GET", "HEAD", "POST"};

    if (!methods.contains(method)) {
        httpResponse.setStatusCode("405 Method Not Allowed");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "unsupported HTTP method: " + string{method})};
    }
}

auto Http::parseGetHead(HttpResponse &httpResponse, const HttpRequest &httpRequest, bool writeBody) -> void {
    Http::parseVersion(httpResponse, httpRequest.getVersion());

    const string_view url{httpRequest.getUrl()};

    span<const byte> body{Http::instance.parseUrl(httpResponse, url)};
    Http::parseTypeEncoding(httpResponse, url);

    Http::parseResource(httpResponse, httpRequest.getHeaderValue("Range"), body, writeBody);
}

auto Http::parseVersion(HttpResponse &httpResponse, string_view version, source_location sourceLocation) -> void {
    if (version != "1.1") {
        httpResponse.setVersion("1.1");
        httpResponse.setStatusCode("505 HTTP Version Not Supported");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "unsupported HTTP version: " + string{version})};
    }

    httpResponse.setVersion(version);
}

auto Http::parseUrl(HttpResponse &httpResponse, string_view url, source_location sourceLocation) const
        -> span<const byte> {
    const auto result{this->resources.find(string{url})};

    if (result == this->resources.end()) {
        httpResponse.setStatusCode("404 Not Found");
        httpResponse.addHeader("Content-Length: 0");
        httpResponse.setBody({});

        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "resource not found: " + string{url})};
    }

    return result->second;
}

auto Http::parseTypeEncoding(HttpResponse &httpResponse, string_view url) -> void {
    if (url.ends_with("html")) {
        httpResponse.addHeader("Content-Type: text/html; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("css")) {
        httpResponse.addHeader("Content-Type: text/css; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("js")) {
        httpResponse.addHeader("Content-Type: text/javascript; charset=utf-8");
        httpResponse.addHeader("Content-Encoding: br");
    } else if (url.ends_with("png"))
        httpResponse.addHeader("Content-Type: image/png");
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

            throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                             Level::WARN, "invalid range: " + string{range})};
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

const Http Http::instance;
