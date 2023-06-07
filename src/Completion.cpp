#include "Completion.h"

using std::shared_ptr;

Completion::Completion(shared_ptr<Ring> &ring) : self{ring->getCompletion()}, ring{ring} {}

Completion::Completion(Completion &&completion) noexcept: self{completion.self}, ring{std::move(completion.ring)} {}

auto Completion::operator=(Completion &&completion) noexcept -> Completion & {
    if (this != &completion) {
        this->self = completion.self;
        this->ring = std::move(completion.ring);
    }
    return *this;
}

auto Completion::getResult() const -> int { return this->self->res; }

auto Completion::getData() const -> unsigned long long { return io_uring_cqe_get_data64(this->self); }

auto Completion::getFlags() const -> unsigned int { return this->self->flags; }

Completion::~Completion() { this->ring->consumeCompletion(this->self); }
