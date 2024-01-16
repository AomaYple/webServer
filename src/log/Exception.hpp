#pragma once

#include "Log.hpp"

class Exception : public std::exception {
public:
    explicit Exception(Log &&log = Log{});

    Exception(const Exception &) = default;

    auto operator=(const Exception &) -> Exception & = default;

    Exception(Exception &&) noexcept = default;

    auto operator=(Exception &&) noexcept -> Exception & = default;

    ~Exception() override = default;

    [[nodiscard]] auto what() const noexcept -> const char * override;

    [[nodiscard]] auto getLog() noexcept -> Log &;

private:
    std::string text;
    Log log;
};
