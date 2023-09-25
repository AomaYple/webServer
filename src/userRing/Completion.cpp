#include "Completion.hpp"

Completion::Completion(const io_uring_cqe *cqe) noexcept : completion{cqe} {}

auto Completion::getUserData() const noexcept -> unsigned long { return io_uring_cqe_get_data64(this->completion); }

auto Completion::getResult() const noexcept -> int { return this->completion->res; }

auto Completion::getFlags() const noexcept -> unsigned int { return this->completion->flags; }
