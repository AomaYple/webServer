#include "Completion.h"

Completion::Completion(io_uring_cqe *cqe) noexcept : completion{cqe} {}

auto Completion::getUserData() const noexcept -> std::uint_fast64_t {
    return io_uring_cqe_get_data64(this->completion);
}

auto Completion::getResult() const noexcept -> std::int_fast32_t { return this->completion->res; }

auto Completion::getFlags() const noexcept -> std::uint_fast32_t { return this->completion->flags; }
