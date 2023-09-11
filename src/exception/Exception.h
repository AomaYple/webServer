#pragma once

#include <stdexcept>

class Exception : public std::exception {
public:
    explicit Exception(std::string &&text) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string text;
};
