#include "Completion.h"

Completion::Completion(io_uring_cqe *cqe) noexcept : self{cqe} {}

Completion::Completion(Completion &&other) noexcept : self{other.self} {}

auto Completion::operator=(Completion &&other) noexcept -> Completion & {
    if (this != &other) this->self = other.self;
    return *this;
}

auto Completion::getUserData() const noexcept -> unsigned long long { return io_uring_cqe_get_data64(this->self); }

auto Completion::getResult() const noexcept -> int { return this->self->res; }

auto Completion::getFlags() const noexcept -> unsigned int { return this->self->flags; }
