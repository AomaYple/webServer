#include "Http.h"

#include "../exception/Exception.h"
#include "../log/Log.h"
#include "Request.h"
#include "Response.h"

#include <brotli/encode.h>
#include <zlib.h>

#include <filesystem>

using std::ifstream, std::ostringstream;
using std::source_location;
using std::string, std::string_view;
using std::filesystem::directory_iterator, std::filesystem::current_path;

Http Http::instance;

auto Http::parse(string_view requestString) -> string {
    Request request{requestString};

    Response response;

    return response.combine();
}

auto Http::gzip(string_view data, source_location sourceLocation) -> string {
    z_stream stream{};

    if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) !=
        Z_OK)
        throw Exception{sourceLocation, Level::FATAL, "gzip initialize error"};

    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    stream.avail_in = data.size();

    string compressed;

    for (std::int_fast32_t result{Z_OK}; result != Z_STREAM_END;) {
        string buffer(1024, 0);

        stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
        stream.avail_out = buffer.size();

        result = deflate(&stream, Z_FINISH);
        if (result != Z_OK && result != Z_STREAM_END)
            throw Exception{sourceLocation, Level::FATAL, "gzip compress error"};

        buffer.resize(stream.total_out);

        compressed += buffer;
    }

    if (deflateEnd(&stream) != Z_OK) throw Exception{sourceLocation, Level::FATAL, "gzip finalize error"};

    return compressed;
}

auto Http::brotli(string_view data, source_location sourceLocation) -> string {
    std::size_t length{BrotliEncoderMaxCompressedSize(data.size())};

    string compressed(length, 0);

    if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE, data.size(),
                              reinterpret_cast<const uint8_t *>(data.data()), &length,
                              reinterpret_cast<uint8_t *>(compressed.data())) != BROTLI_TRUE)
        throw Exception{sourceLocation, Level::FATAL, "brotli compress error"};

    compressed.resize(length);

    return compressed;
}

Http::Http() {
    this->webs.emplace("", "");

    for (const auto &webPath: directory_iterator(current_path().string() + "/web")) {
        ifstream webFile{webPath.path()};

        ostringstream webFileStream;
        webFileStream << webFile.rdbuf();

        if (webPath.path().filename().string().ends_with("html")) {
            this->webs.emplace(webPath.path().filename().string() + ".gz", Http::gzip(webFileStream.str()));

            this->webs.emplace(webPath.path().filename().string() + ".br", Http::brotli(webFileStream.str()));
        }

        this->webs.emplace(webPath.path().filename(), webFileStream.str());
    }
}
