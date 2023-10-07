#pragma once

#include "log.hpp"

class exception : std::exception {
public:
    explicit exception(log &&log) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

private:
    std::string message;
    log log;
};
