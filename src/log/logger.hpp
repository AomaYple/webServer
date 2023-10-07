#pragma once

#include "node.hpp"

#include <fstream>

class logger {
    logger();

public:
    logger(const logger &) = delete;

    logger(logger &&) = delete;

    auto operator=(const logger &) -> logger & = delete;

    auto operator=(logger &&) -> logger & = delete;

    ~logger();

    static auto start() -> void;

    static auto stop() noexcept -> void;

private:
    auto run(const std::stop_token &stop_token) -> void;

    [[nodiscard]] static auto invert_linked_list(node *pointer) noexcept -> node *;

    auto consume(node *pointer) -> void;

public:
    static auto produce(log &&log) -> void;

private:
    static logger instance;

    std::ofstream log_file;
    std::atomic<node *> head;
    std::atomic_flag notice;
    std::jthread worker;
};
