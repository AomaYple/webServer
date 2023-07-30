#pragma once

#include "../log/Message.h"

class HttpParseError : public std::exception {
public:
    HttpParseError(std::source_location sourceLocation, std::string &&information);

    [[nodiscard]] auto what() const noexcept -> const char * override;

    auto getMessage() noexcept -> Message;

private:
    Message message;
    std::string messageString;
};
