#include "Completion.h"

Completion::Completion(io_uring_cqe *cqe) noexcept : completion{cqe} {}

auto Completion::getUserData() const noexcept -> __u64 { return io_uring_cqe_get_data64(this->completion); }

auto Completion::getResult() const noexcept -> __s32 { return this->completion->res; }

auto Completion::getFlags() const noexcept -> __u32 { return this->completion->flags; }
