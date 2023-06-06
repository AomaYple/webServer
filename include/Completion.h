#pragma once

#include <liburing.h>

class Completion {
    friend class Ring;

public:
    Completion(const Completion &completion) = delete;

    Completion(Completion &&completion) noexcept;

    auto operator=(Completion &&completion) noexcept -> Completion &;

    [[nodiscard]] auto getResult() const -> int;

    [[nodiscard]] auto getData() const -> void *;

    [[nodiscard]] auto getFlags() const -> unsigned int;

private:
    explicit Completion(io_uring_cqe *cqe);

    io_uring_cqe *self;
};
