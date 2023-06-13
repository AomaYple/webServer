#pragma once

#include <liburing.h>

class Completion {
    friend class Ring;

public:
    Completion(const Completion &other) = delete;

    Completion(Completion &&other) noexcept;

    auto operator=(Completion &&other) noexcept -> Completion &;

    [[nodiscard]] auto getResult() const -> int;

    [[nodiscard]] auto getData() const -> unsigned long long;

    [[nodiscard]] auto getFlags() const -> unsigned int;

private:
    explicit Completion(io_uring_cqe *completion);

    io_uring_cqe *self;
};
