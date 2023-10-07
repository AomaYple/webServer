#pragma once

#include <string>

class database_call_error : std::exception {
public:
    explicit database_call_error(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
