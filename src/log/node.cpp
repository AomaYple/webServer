#include "node.hpp"

node::node(class log &&log, node *next) noexcept : log{std::move(log)}, next{next} {}
