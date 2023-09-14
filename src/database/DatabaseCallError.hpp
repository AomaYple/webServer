#pragma once

#include <string>

class DatabaseCallError : public std::exception {
public:
    explicit DatabaseCallError(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
