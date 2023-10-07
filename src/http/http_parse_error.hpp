#pragma once

#include "../log/log.hpp"

class http_parse_error : std::exception {
public:
    explicit http_parse_error(log &&log) noexcept;

    [[nodiscard]] auto what() const noexcept -> const char * override;

    [[nodiscard]] auto get_log() noexcept -> log;

private:
    std::string message;
    log log;
};
