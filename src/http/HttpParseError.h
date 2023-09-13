#pragma once

#include <string>

class HttpParseError : public std::exception {
public:
    explicit HttpParseError(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
