#pragma once

#include <liburing.h>

class Ring;

class Completion {
public:
    explicit Completion(io_uring_cqe *submission);

    Completion(const Completion &other) = delete;

    Completion(Completion &&other) noexcept;

    auto operator=(Completion &&other) noexcept -> Completion &;

    [[nodiscard]] auto getData() const -> unsigned long long;

    [[nodiscard]] auto getResult() const -> int;

    [[nodiscard]] auto getFlags() const -> unsigned int;

private:
    io_uring_cqe *self;
};
