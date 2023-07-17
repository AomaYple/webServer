#include "Completion.h"

Completion::Completion(io_uring_cqe *cqe) noexcept : completion{cqe} {}

Completion::Completion(Completion &&other) noexcept : completion{other.completion} {}

auto Completion::operator=(Completion &&other) noexcept -> Completion & {
    if (this != &other) this->completion = other.completion;
    return *this;
}

auto Completion::getUserData() const noexcept -> unsigned long long {
    return io_uring_cqe_get_data64(this->completion);
}

auto Completion::getResult() const noexcept -> int { return this->completion->res; }

auto Completion::getFlags() const noexcept -> unsigned int { return this->completion->flags; }
