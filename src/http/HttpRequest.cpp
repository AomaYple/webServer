#include "HttpRequest.hpp"

HttpRequest::HttpRequest(std::string_view request) {
    bool isParsedLine{}, isParsedBody{};

    for (unsigned long lineBreak{request.find("\r\n")}; lineBreak != std::string_view::npos && !isParsedBody;
         lineBreak = request.find("\r\n")) {
        if (!isParsedLine) {
            isParsedLine = true;
            this->parseLine(request.substr(0, lineBreak));

            request.remove_prefix(lineBreak + 2);
        } else [[unlikely]] if (lineBreak == 0) {
            isParsedBody = true;
            this->body = request.substr(2);
        } else {
            this->parseHeader(request.substr(0, lineBreak));

            request.remove_prefix(lineBreak + 2);
        }
    }
}

auto HttpRequest::getVersion() const noexcept -> std::string_view { return this->version; }

auto HttpRequest::getMethod() const noexcept -> std::string_view { return this->method; }

auto HttpRequest::getUrl() const noexcept -> std::string_view { return this->url; }

auto HttpRequest::containsHeader(const std::string_view filed) const -> bool { return this->headers.contains(filed); }

auto HttpRequest::getHeaderValue(const std::string_view filed) const -> std::string_view {
    return this->headers.at(filed);
}

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

auto HttpRequest::parseHeader(const std::string_view header) -> void {
    const auto splitPoint{header.cbegin() + header.find(": ")};

    this->headers.emplace(std::string_view{header.cbegin(), splitPoint},
                          std::string_view{splitPoint + 2, header.cend()});
}
