#pragma once

#include <source_location>
#include <string>

class DatabaseCallError : public std::exception {
public:
    DatabaseCallError(std::source_location sourceLocation, std::string &&information);

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string messageString;
};
