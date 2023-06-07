#pragma once

#include <memory>

#include "Ring.h"

class Completion {
public:
    explicit Completion(std::shared_ptr<Ring> &ring);

    Completion(const Completion &completion) = delete;

    Completion(Completion &&completion) noexcept;

    auto operator=(Completion &&completion) noexcept -> Completion &;

    [[nodiscard]] auto getResult() const -> int;

    [[nodiscard]] auto getData() const -> unsigned long long;

    [[nodiscard]] auto getFlags() const -> unsigned int;

    ~Completion();

private:
    io_uring_cqe *self;
    std::shared_ptr<Ring> ring;
};
