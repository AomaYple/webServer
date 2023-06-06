#include "Completion.h"

Completion::Completion(io_uring_cqe* cqe) : self{cqe} {}

Completion::Completion(Completion&& completion) noexcept : self{completion.self} {}

auto Completion::operator=(Completion&& completion) noexcept -> Completion& {
    if (this != &completion) this->self = completion.self;
    return *this;
}

auto Completion::getResult() const -> int { return this->self->res; }

auto Completion::getData() const -> void* { return io_uring_cqe_get_data(this->self); }

auto Completion::getFlags() const -> unsigned int { return this->self->flags; }
