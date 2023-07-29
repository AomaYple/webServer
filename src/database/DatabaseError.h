#pragma once

#include <source_location>
#include <string>

class DatabaseError : public std::exception {
public:
    DatabaseError(std::source_location sourceLocation, std::string &&information);

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
