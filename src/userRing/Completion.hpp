#pragma once

#include <liburing.h>

class Completion {
public:
    explicit Completion(const io_uring_cqe *cqe) noexcept;

    Completion(const Completion &) = delete;

    Completion(Completion &&) = default;

    auto operator=(const Completion &) -> Completion & = delete;

    auto operator=(Completion &&) noexcept -> Completion & = default;

    ~Completion() = default;

    [[nodiscard]] auto getUserData() const noexcept -> unsigned long;

    [[nodiscard]] auto getResult() const noexcept -> int;

    [[nodiscard]] auto getFlags() const noexcept -> unsigned int;

private:
    const io_uring_cqe *completion;
};
