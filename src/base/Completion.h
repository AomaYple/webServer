#pragma once

#include <liburing.h>

class Completion {
public:
    explicit Completion(io_uring_cqe *cqe) noexcept;

    Completion(const Completion &) = delete;

    Completion(Completion &&) noexcept = default;

    [[nodiscard]] auto getUserData() const noexcept -> __u64;

    [[nodiscard]] auto getResult() const noexcept -> __s32;

    [[nodiscard]] auto getFlags() const noexcept -> __u32;

private:
    const io_uring_cqe *const completion;
};
