#pragma once

#include <string>

class Exception : public std::exception {
public:
    explicit Exception(std::string_view text) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string text;
};
