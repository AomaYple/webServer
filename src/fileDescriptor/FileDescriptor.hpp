#pragma once

#include "../ring/Ring.hpp"

#include <memory>

class FileDescriptor {
public:
    FileDescriptor(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept;

    FileDescriptor(const FileDescriptor &) = delete;

    auto operator=(const FileDescriptor &) -> FileDescriptor & = delete;

    FileDescriptor(FileDescriptor &&) noexcept = default;

    auto operator=(FileDescriptor &&) -> FileDescriptor & = delete;

    ~FileDescriptor() = default;

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

    auto cancel() const -> void;

    auto close() const -> void;

protected:
    [[nodiscard]] auto getRing() const noexcept -> std::shared_ptr<Ring>;

private:
    const int fileDescriptor;
    std::shared_ptr<Ring> ring;
};
