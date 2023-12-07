#pragma once

#include "Log.hpp"

class Exception : std::exception {
public:
    explicit Exception(Log &&log) noexcept;

    Exception(const Exception &) = default;

    auto operator=(const Exception &) -> Exception & = default;

    Exception(Exception &&) = default;

    auto operator=(Exception &&) -> Exception & = default;

    ~Exception() override = default;

    [[nodiscard]] auto what() const noexcept -> const char * override;

    [[nodiscard]] auto getLog() noexcept -> Log &;

private:
    std::string message;
    Log log;
};
