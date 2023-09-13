#pragma once

#include <string>

class SystemCallError : public std::exception {
public:
    explicit SystemCallError(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
