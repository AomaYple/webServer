#pragma once

#include "../ring/Ring.hpp"

#include <memory>

class FileDescriptor {
public:
    FileDescriptor(int fileDescriptor, std::shared_ptr<Ring> ring) noexcept;

    FileDescriptor(const FileDescriptor &) = delete;

    auto operator=(const FileDescriptor &) -> FileDescriptor & = delete;

    FileDescriptor(FileDescriptor &&) noexcept;

    auto operator=(FileDescriptor &&) noexcept -> FileDescriptor &;

    ~FileDescriptor();

    [[nodiscard]] auto getFileDescriptor() const noexcept -> int;

protected:
    [[nodiscard]] auto getRing() const noexcept -> std::shared_ptr<Ring>;

private:
    auto close() const -> void;

    int fileDescriptor;
    std::shared_ptr<Ring> ring;
};
