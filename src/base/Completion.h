#pragma once

#include <liburing.h>

class Completion {
public:
    explicit Completion(io_uring_cqe *cqe) noexcept;

    Completion(const Completion &) = delete;

    [[nodiscard]] auto getUserData() const noexcept -> unsigned long long;

    [[nodiscard]] auto getResult() const noexcept -> int;

    [[nodiscard]] auto getFlags() const noexcept -> unsigned int;

private:
    io_uring_cqe *completion;
};