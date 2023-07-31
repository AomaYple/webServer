#pragma once

#include "../log/Message.h"

class Exception : public std::exception {
public:
    Exception(std::source_location sourceLocation, Level level, std::string &&information);

    [[nodiscard]] auto what() const noexcept -> const char * override;

    auto getMessage() noexcept -> Message;

private:
    Message message;
    std::string messageString;
};
