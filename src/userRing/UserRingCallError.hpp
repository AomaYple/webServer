#pragma once

#include <string>

class UserRingCallError : public std::exception {
public:
    explicit UserRingCallError(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
