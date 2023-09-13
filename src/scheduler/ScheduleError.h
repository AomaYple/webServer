#pragma once

#include <string>

class ScheduleError : public std::exception {
public:
    explicit ScheduleError(std::string &&message) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
};
