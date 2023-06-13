#include "Completion.h"

Completion::Completion(io_uring_cqe *completion) : self{completion} {}

Completion::Completion(Completion &&other) noexcept : self{other.self} {}

auto Completion::operator=(Completion &&other) noexcept -> Completion & {
    if (this != &other) this->self = other.self;
    return *this;
}

auto Completion::getResult() const -> int { return this->self->res; }

auto Completion::getData() const -> unsigned long long { return io_uring_cqe_get_data64(this->self); }

auto Completion::getFlags() const -> unsigned int { return this->self->flags; }
