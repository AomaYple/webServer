#pragma once

#include "log.hpp"

struct node {
    node(log &&log, node *next) noexcept;

    node(const node &) = delete;

    node(node &&) = default;

    auto operator=(const node &) -> node & = delete;

    auto operator=(node &&) -> node & = default;

    ~node() = default;

    log log;
    node *next;
};