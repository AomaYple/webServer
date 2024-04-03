#include "HttpRequest.hpp"

HttpRequest::HttpRequest(std::string_view request) {
    bool parsedLine{}, parsedBody{};

    for (unsigned long result{request.find("\r\n")}; result != std::string_view::npos && !parsedBody;
         result = request.find("\r\n")) {
        if (!parsedLine) {
            parsedLine = true;
            this->parseLine(request.substr(0, result));

            request.remove_prefix(result + 2);
        } else if (result == 0) {
            parsedBody = true;
            this->body = request.substr(2);
        } else {
            this->parseHeader(request.substr(0, result));

            request.remove_prefix(result + 2);
        }
    }
}

auto HttpRequest::getVersion() const noexcept -> std::string_view { return this->version; }

auto HttpRequest::getMethod() const noexcept -> std::string_view { return this->method; }

auto HttpRequest::getUrl() const noexcept -> std::string_view { return this->url; }

auto HttpRequest::containsHeader(std::string_view filed) const -> bool { return this->headers.contains(filed); }

auto HttpRequest::getHeaderValue(std::string_view filed) const -> std::string_view { return this->headers.at(filed); }

auto HttpRequest::getBody() const noexcept -> std::string_view { return this->body; }

auto HttpRequest::parseLine(std::string_view line) noexcept -> void {
    {
        const unsigned long space{line.find(' ')};
        this->method = line.substr(0, space);
        line.remove_prefix(space + 1);
    }

    {
        const unsigned long space{line.find(' ')};
        this->url = line.substr(0, space);
        line.remove_prefix(space + 1);
    }

    this->version = line;
}

auto HttpRequest::parseHeader(std::string_view header) -> void {
    const auto point{header.cbegin() + header.find(": ")};
    const std::string_view key{header.cbegin(), point}, value{point + 2, header.cend()};

    this->headers.emplace(key, value);
}
