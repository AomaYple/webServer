#pragma once

#include <liburing.h>

#include <cstdint>

class Completion {
public:
    explicit Completion(io_uring_cqe *cqe) noexcept;

    Completion(const Completion &) = delete;

    [[nodiscard]] auto getUserData() const noexcept -> std::uint_fast64_t;

    [[nodiscard]] auto getResult() const noexcept -> std::int_fast32_t;

    [[nodiscard]] auto getFlags() const noexcept -> std::uint_fast32_t;

private:
    io_uring_cqe *completion;
};
